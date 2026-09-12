# Yeelight I²C front panel

Several Yeelight lamps put their power button, mode button and brightness
slider on a small microcontroller of their own rather than wiring them to the
ESP32 directly. That controller is an I²C slave, and it also drives the LEDs
that illuminate the panel. Because there is no stock ESPHome component that can
speak to it, these lamps need the `yeelight_front_panel` component.

Lamps whose front panel is a plain GPIO button (`lamp9`, most ceiling lights)
do **not** need this component — a `binary_sensor: gpio` is enough.

## How the panel works

The exchange is the same on every model found so far:

1. The panel pulls a GPIO line low when an event is waiting.
2. The ESP32 writes a fixed request message (on models that need one).
3. The ESP32 reads a message back and decodes it.

LED updates go the other way: the ESP32 writes a message containing a 16 bit
bitmask of the LEDs that should be lit.

The component keeps the interrupt handler to a single counter increment and
does all I²C work from the main loop, so a slow bus can never block the ISR.

## Supported models

### `bslamp2` — Xiaomi Mijia Bedside Lamp 2

Address `0x2C`, 7 byte messages, 10 slider LEDs, 22 slider levels.

Event request: `01:00:00:00:00:00:01`

| Byte | Meaning |
|---|---|
| 0-3 | fixed prefix `04:04:01:00` |
| 4 | part — `01` power button, `02` color button, `03` slider touch, `04` slider release |
| 5 | buttons: `01` touch, `02` release. Slider: position, counting down from `0x16` |
| 6 | checksum: `message[5] + message[4] + 1` |

LED message: `02:03:<state_hi>:<state_lo>:64:00:00`, where bits 10 and 11 of the
state must always be set, bit 14 is the power button LED, bit 12 the color
button LED, and bits 9 down to 0 are slider LEDs 1 to 10.

This protocol was originally worked out by the
[esphome-xiaomi_bslamp2](https://github.com/mmakaay/esphome-xiaomi_bslamp2)
project. The implementation here is independent of theirs.

### `lamp10` — Yeelight Staria Floor Lamp

Address `0x50`, SDA GPIO21, SCL GPIO19, trigger GPIO16. LED messages are
3 bytes, events are read as 6 bytes. 22 slider levels.

The LED message comes from static analysis of the original firmware: the front
panel driver writes three bytes to slave `0x50` on I²C port 1, and caches a
16 bit LED state into the last two bytes immediately before the write. So the
LED message is `<command>:<state_hi>:<state_lo>`. The command byte and the
slider LED count are still unverified.

The event layout was captured on the device (2026-09-12). No request message is
needed; a plain 6 byte read returns:

| Byte | Meaning |
|---|---|
| 0 | fixed `0A` |
| 1 | power button (bottom): `01` touch, `02` held (repeats), `03` release |
| 2 | colour button (top): same as the power button |
| 3 | slider: `01` touch and while moving, `02` release |
| 4 | slider position, `00` at the "−" end .. `15` at the "+" end |
| 5 | always `00` |

This panel behaves differently from the Bedside Lamp 2 in three ways that the
model has to handle:

* **It reports state, not events.** Every read returns the whole panel state,
  and stale fields are never cleared — after a button press the slider still
  shows its last state and position. `parse_event()` therefore only emits an
  event for a field that changed since the previous message, with the original
  firmware's precedence: colour button over power button over slider.
* **The trigger line can stay low.** A falling-edge interrupt alone loses
  events, so the hub keeps reading every 50 ms while the line is low. Reads
  with nothing pending return `0A:00:00:00:00:00`.
* **The position sent with a release is meaningless** (`06`, `0B` or `0C`
  regardless of where the finger left). Only touches carry a slider level.
* **A button release can go unreported.** The panel was seen dropping a held
  button straight to `00` and releasing the trigger line without a `03` ever
  being read. A drop from `01`/`02` to `00` therefore counts as the release,
  and the hub reads once more when the trigger line goes back high.

A 3 byte read, which the original analysis suggested, returns the first three
bytes only and loses the position.

Not handled yet: a slider touch registered briefly next to a button press. The
original firmware suppresses touches right after a button action
(`touch_act_delay`, "ignore the touch action").

## Adding a model

Everything model specific lives behind `FrontPanelModel` in `models.h`. To add
a lamp:

1. Add a class implementing `message_length()`, `parse_event()` and
   `encode_leds()`, plus the slider LED and level counts.
2. Add it to the `Model` enum and to `get_model()`.
3. Add the name and default address to `MODELS` in `__init__.py`.

Nothing above that interface changes — the binary sensors and the slider sensor
work on the decoded `FrontPanelEvent`, never on raw bytes.

### Known but not yet implemented

The **Yeelight Staria Table Lamp** (`lamp9`) also has an I²C front panel, on a
different address again, using 7 byte messages with a `0a` prefix and 22 slider
levels. Note that `yeelight_light_lamp9.yaml` in this repository drives that
lamp's plain GPIO button instead and leaves the panel unused.
