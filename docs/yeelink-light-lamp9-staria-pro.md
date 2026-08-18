# Yeelight Staria Bedside Lamp Pro (YLCT03YL, `yeelink.light.lamp9`)

Hardware notes for the Pro variant, gathered by probing and tracing one unit.

**Reference unit:** production date 2020-10, module ESP32-WROOM-32D,
stock firmware `2.1.7_0031`, `miio_ver 0.0.9`.

Only this one unit was examined. The Pro and the non-Pro Staria both report the
internal model `yeelink.light.lamp9`, so the model string alone does not tell you
which board is inside.

---

## Status LED is an AND gate across two boards

The base `yeelink.light.lamp9` config lists GPIO33 as `LED (YLCT03YL only)` and
drives it as a plain output. That is correct as far as it goes, but it will not
light the LED on its own, and the reason is not obvious from software.

The Pro has two boards - the ESP board and the Qi charger board - joined by
exactly three wires: **GND**, **+12V**, and one signal pad labelled **`GPIO-WL`**
on the ESP board. The status LED itself sits on the ESP board.

```
   V+ ──┬─────────────────── emitter ──┐
        │                              │
       R43                          [R1A]   PNP, high side
        │                              │
        └──── base ◄── R44 ◄── R? ◄── collector ── [6C] NPN #2
                                                        │
              ESP32 GPIO33 ── R16 (4.7k) ──────► base ──┤
                                                        │
                                                    emitter ──► GND
                              │
                         collector (pin 3)
                              │
                            R42 (2k)
                              │
                            LED1
                              │
                         collector ── [6C] NPN #1
                                          │
  charger LED out ──► GPIO-WL ──► R40 ──┬─► base
                                        │     │
                                     R41 (8.2k)│
                                        │  emitter ──► GND
                                       GND
                                        │
                                       C37
```

Both three-pin parts are SOT-23. `6C` matches the marking code for a BC817 NPN.
The part marked `R1A` was not identified; its role as a high-side switch is
inferred from where its legs go, not from the marking. The component between
NPN #2 and R44 reads above 20k and was not identified either.

**Both halves must conduct for the LED to light:**

| Half | Driven by | Role |
| ---- | --------- | ---- |
| High side (`R1A` via NPN #2, from GPIO33) | **ESP32** | enables the LED supply rail |
| Low side (NPN #1, from `GPIO-WL`) | **Qi charger** | drives the indication pattern |

So GPIO33 is an **enable**, not a drive. The charger decides the pattern by
itself; the ESP32 only permits or suppresses it. Observed behaviour:

| Charger state | LED |
| ------------- | --- |
| phone charging | steady on |
| phone present but not aligned | blinking |
| no phone | off |

This is easy to misdiagnose. Toggling the GPIO33 light entity with no phone on
the charger does nothing at all, which looks exactly like a dead LED or a wrong
pin. Verify with a phone charging **and** the enable on.

### Practical consequence

You can suppress the charge indicator (useful at night) by turning the enable
off. You cannot make the LED light on demand, and you cannot change its pattern.

## The ESP32 cannot see charge state

There is no path from the charger to any ESP32 pin. The charger's only signal
(`GPIO-WL`) terminates at the base of NPN #1; that node connects to nothing else
on the board. Verified by continuity against every WROOM castellation.

Also checked and found empty:

- every free GPIO sampled with deterministic pull-ups, while a phone was placed,
  moved off-centre and removed - no pin changed state
- both plausible I2C buses. GPIO17/18 has one device at `0x10`, whose full 256
  register space reads `0x00` and never changes. GPIO21/22 is empty

Charge state therefore cannot be exposed to Home Assistant without modifying the
hardware. GPIO13, GPIO15, GPIO16, GPIO19 and GPIO23 were found unused on this
board, but no modification was attempted or tested.

## White channels will not dim below ~6% duty

The constant-current driver stops conducting reliably under roughly **6% PWM
duty** on this unit. Measured by driving the raw LEDC channels directly and
raising the duty until the LEDs lit steadily.

Symptoms when this is not compensated for:

- at or below ~4% brightness the light goes fully dark
- at ~5% it lights only near the colour temperature extremes; moving toward the
  middle makes it flicker and then drop out

The second symptom is the giveaway. With `constant_brightness`, mid-range colour
temperature splits the requested brightness across both channels, so each one
falls under the threshold even though their sum does not.

### Why `min_power` alone is not the fix

`min_power` lifts *every* non-zero value, and `zero_means_zero` only catches an
exact `0.0`. The light component does not produce an exact zero at the colour
temperature extremes: with the range declared as 2700K-6500K, a request for the
warm end arrives rounded in mireds and leaves the cold channel at about **0.17%**
rather than 0.

```
ct_ratio = (370 - 153.846) / (370.370 - 153.846) = 0.99829
  -> warm 99.83%, cold 0.171%
```

`min_power` raises that residual to the full 6% floor, and since the cold LEDs
are brighter per unit duty, "maximum warm" comes out visibly cold.

The config uses template outputs with a **deadband** instead: below `deadband`
the channel is genuinely off, at or above it the floor applies.

### Range floor

At the colour temperature extremes one channel carries everything, so the dimmest
usable output is around 6%. Mid-range both channels run, so it is roughly 12%
combined. The night light uses a different driver, dims cleanly below 1%, and is
the right output for very low light.

Measuring the floor on your own unit: drive `pwm_warm` and `pwm_cold` directly
with `output.set_level` and raise the value until the LED lights without flicker.
Note that `output.set_level` on a wrapped output goes through the same mapping as
the light, so measure against the raw `ledc` outputs, not the template ones.

## PWM frequency

ESPHome's LEDC default of 1 kHz at 16-bit resolution is not the limiting factor -
6% duty is thousands of steps in, nowhere near quantisation. The floor is the LED
driver's conduction threshold. No other frequency was tested.

## Bootloader note when flashing over the network

Flashing over the air replaces only the application partition; the stock
bootloader and partition table stay in place. ESPHome reports:

```
[W][app:190]: Bootloader too old for OTA rollback and SRAM1 as IRAM (+40KB).
[C][safe_mode:079]:   Bootloader rollback: not supported
```

Harmless in practice, but it costs around 40KB of IRAM and there is no OTA
rollback protection. Only a UART flash replaces the bootloader.

## Transition length

`default_transition_length` is a compile-time setting in ESPHome, so the config
also exposes a `transition length` number entity that calls
`set_default_transition_length()` on both lights at runtime. The value is stored
on the device and reapplied on boot - restoring a template number does not re-run
its `set_action`, so without that the entity would show a value the lights were
not using.

Home Assistant can also override the fade per call with the `transition`
parameter of `light.turn_on` / `light.turn_off`, which takes precedence over the
default.

## Brightness across colour temperature

ESPHome normalises the two channel fractions so `max(cw, ww) == 1`, then
`constant_brightness` decides how they are combined. At 100% brightness:

| Colour temp | `constant_brightness: true` | `constant_brightness: false` |
| ----------- | --------------------------- | ---------------------------- |
| 2700K warm  | one channel 100%, sum 100%  | one channel 100%, sum 100%   |
| 3815K       | each 50%, sum 100%          | **both 100%, sum 200%**      |
| 6500K cold  | one channel 100%, sum 100%  | one channel 100%, sum 100%   |

With `constant_brightness: true` the sum is held constant across the whole
range, so both channels can never run at full and the lamp tops out at one
channel's worth of light. This config uses **false**, which makes the mired
midpoint the maximum-output point - both channels at 100% - matching the stock
firmware. The cost is that brightness varies as colour temperature moves, which
the stock lamp also does.

That midpoint is 3815K, and it is worth knowing where it appears in Home
Assistant: mireds are reciprocal to Kelvin, and the HA slider is linear in
Kelvin, so the mired midpoint sits at about **29%** of a 2700-6500K slider
rather than halfway along it.

`constant_brightness: true` is also what made the low-brightness dropout worse:
splitting the requested brightness across both channels at mid colour
temperature put each one under the driver's conduction floor.

## How the device is identified in Home Assistant

Without help, Home Assistant labels the device *esp32doit-devkit-v1 by Espressif*.
The API reports `ESPHOME_BOARD` as the model and a hardcoded per-platform string
as the manufacturer, and neither is configurable.

Home Assistant does, however, split `project.name` on its single dot and use the
halves, so the config sets:

```yaml
esphome:
  project:
    name: "Yeelight.Staria Bedside Lamp Pro"
    version: "1.0"
```

which yields `manufacturer=Yeelight`, `model=Staria Bedside Lamp Pro`, and a
`sw_version` of `1.0 (ESPHome <version>)`.
