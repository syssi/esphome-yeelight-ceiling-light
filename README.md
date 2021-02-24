# esphome-yeelight-ceiling-light

Esphome custom firmware for the Yeelight Ceiling Light (YLXD76YL).


## Supported devices

| Name                                             | Model                  | Model no.   | Specs                                   |
| ------------------------------------------------ | ---------------------- | ----------- | --------------------------------------- |
| Yeelight Ceiling Light                           | yeelink.light.ceil26   | YLXD76YL    | AC220V, 23W, 1500lm, 2700K-6500K, 32cm  |
| Yeelight XianYu C2001C550 50W AC220V (untested!) | yeelink.light.ceil26   | C2001C550   | AC220V, 50W, 2700K-6500K, 55.5cm        |


## Features

- Light (CCWW)
  - Brightness
  - Color temperature (2700K-6500K)
- Nightlight (2700K)
  - Brightness
- Sensor
  - Power supply (ESP32 VCC)


## TODOs

- Interlock light and nightlight mode
- Bluethooth gateway ([Known bugs](#known-bugs))

## Known bugs

- Bootloop when BLE Tracker configured: https://github.com/esphome/issues/issues/1731

## GPIOs

| Name                | Label  | ESP32 GPIO   |
| ------------------- | ------ | ------------ |
| Warm white PWM      | W      | GPIO19       |
| Cold white PWM      | C      | GPIO21       |
| Night light PWM     | NL     | GPIO23       |
| Power supply GPIO   | STB    | GPIO23       |
| VCC measurement     | ADC1   | GPIO35       |
| TXD                 |        | GPIO1        |
| RXD                 |        | GPIO3        |
| GPIO0               | TP3    | GPIO0        |

The ESP32 will enter the serial bootloader when GPIO0 (test point TP3) is held low (GND) on reset / power.
