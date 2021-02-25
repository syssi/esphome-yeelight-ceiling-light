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
  - Power supply voltage (ESP32 VCC)


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

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/012.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/012.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/011.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/011.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/007.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/010.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/010.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/008.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/008.jpg" width="18%">
</a>
