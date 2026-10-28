# Yeelight I²C front panel

The Yeelight Staria Floor Lamp (`yeelink.light.lamp10`) puts its power button,
colour button and brightness slider on a small microcontroller of its own
rather than wiring them to the ESP32 directly. That controller is an I²C slave,
and it also drives the LEDs that illuminate the panel. Because there is no
stock ESPHome component that can speak to it, the lamp needs the
`yeelight_front_panel` component.

Lamps whose front panel is a plain GPIO button (`lamp9`, most ceiling lights)
do **not** need this component — a `binary_sensor: gpio` is enough.

Inspired by [esphome-xiaomi_bslamp2](https://github.com/mmakaay/esphome-xiaomi_bslamp2).

## How the panel works

1. The panel pulls a GPIO line low when an event is waiting.
2. The ESP32 reads a message back and decodes it.

LED updates go the other way: the ESP32 writes a message containing a 16 bit
bitmask of the LEDs that should be lit.

The component keeps the interrupt handler to a single counter increment and
does all I²C work from the main loop, so a slow bus can never block the ISR.

## Protocol

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

The component has to handle a few quirks of this panel:

* **It reports state, not events.** Every read returns the whole panel state,
  and stale fields are never cleared — after a button press the slider still
  shows its last state and position. The component therefore only emits an
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

## Known but not yet implemented

The **Yeelight Staria Table Lamp** (`lamp9`) also has an I²C front panel, on a
different address again, using 7 byte messages with a `0a` prefix and 22 slider
levels. Note that `yeelight_light_lamp9.yaml` in this repository drives that
lamp's plain GPIO button instead and leaves the panel unused.
