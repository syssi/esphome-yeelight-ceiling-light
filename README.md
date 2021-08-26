# esphome-yeelight-ceiling-light

Esphome custom firmware for some Yeelight Ceiling Lights.


## Supported devices

| Name                                             | Model                     | Model no.   | Specs                                   |
| ------------------------------------------------ | ------------------------- | ----------- | --------------------------------------- |
| Yeelight Ceiling Light YLXD76YL                  | yeelink.light.ceil26      | YLXD76YL    | AC220V, 23W, 1500lm, 2700K-6500K, 32cm  |
| Yeelight XianYu C2001C550                        | yeelink.light.ceil26      | C2001C550   | AC220V, 50W, 2700K-6500K, 55.5cm        |
| Yeelight XianYu C2001C450 (untested!)            | yeelink.light.ceil26(?)   | C2001C450   | AC220V, 50W, 2700K-6500K, 45.5cm        |
| Yeelight XianYu C2001S500 (untested!)            | yeelink.light.ceil26(?)   | C2001S500   | AC220V, 50W, 2700K-6500K, 50.5x50.5cm   |
| Yeelight Meteorite Ceiling Light                 | yeelink.light.ceiling10   | YLDL01YL    | AC220V, 33W, 2700K-6500K, 7W RGB ambient light, 90x7x4 cm  |
| Yeelight Ceiling Light YLXD42YL                  | yeelink.light.ceiling15   | YLXD42YL    | AC220V, 32W, 2200lm, 2700K-6500K, 48cm  |
| Yeelight Ceiling Light YLXD41YL                  | yeelink.light.ceilingUNKNOWN  | YLXD41YL    | AC220V, 28W, 1800lm, 2700K-6500K, 32cm  |
| Yeelight Staria Bedside LED                      | yeelink.light.lamp9       | YLCT02YL    | AC220V, 20W, 350lm, 2700K-6500K, without charger |
| Yeelight Staria Pro Bedside LED                  | yeelink.light.lampUNKNOWN | YLCT03YL    | AC220V, 20W, 350lm, 2700K-6500K, Qi charger      |
| Mi Smart LED Ceiling Light                       | yeelink.light.ceiling22   | MJXDD01     | AC220V, 45W, 3100lm, 2700K-6000K, 45cm    |
| Yeelight Ceiling Light YLXD50YL                  | yeelink.light.ceiling20   | YLXD50YL    | AC220V, 50W, 3100lm, 2700K-6500K, RGB ambient light, 47cm  |

### More esphome + yeelight projects

- [Yeelight LED Screen Light Bar (YLTD001)](https://github.com/dckiller51/esphome-yeelight-led-screen-light-bar)
- [Yeelight Ceiling Light YLXD41YL](https://github.com/jaddel/ESPHome-Configurations/tree/master/Devices/Ceiling%20Yeelight%20YLXD41YL)
- [Xiaomi Mi Desk Lamp 1S (ESP32 based)](https://www.reddit.com/r/Esphome/comments/j11ayw/working_configuration_for_xiaomi_mi_desk_lamp_1s/)
- [Xiaomi Mi Desk Lamp (MJTD01YL, ESP8266 based)](https://github.com/syssi/esphome-mi-desk-lamp)
- [Xiaomi Philips Light](https://github.com/syssi/esphome-xiaomi-philips-light)


## Features

### yeelink.light.ceil26, yeelink.light.ceiling22

- Light (CCWW)
  - Brightness
  - Color temperature (2700K-6500K)
- Nightlight (2700K)
  - Brightness
- Sensor
  - Power supply voltage (ESP32 VCC)

### yeelink.light.ceiling10, yeelink.light.ceiling20

- Light (CCWW)
  - Brightness
  - Color temperature (2700K-6500K)
- Nightlight (2700K)
  - Brightness
- Ambient light  (RGB)
  - Brightness
  - Color
- Sensor
  - ADC1
  - ADC2

### yeelink.light.ceiling15, yeelink.light.ceilingUNKNOWN

- Light (CCWW)
  - Brightness
  - Color temperature (2700K-6500K)
- Nightlight (2700K)
  - Brightness
- Sensor
  - ADC1
  - ADC2

### yeelink.light.lamp9, yeelink.light.lampX

- Power button
- Light (CCWW)
  - Brightness
  - Color temperature (2700K-6500K)
- Nightlight (2700K)
  - Brightness

## Known bugs

Bootloop when BLE Tracker configured. Luckily ESPHome goes into safe mode after 10 unsuccessful boot attempts by default. So the device can be flashed OTA nevertheless.

<details>
  <summary>Bootlog</summary>

```
[11:59:23]ets Jun  8 2016 00:22:57
[11:59:23]
[11:59:23]rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
[11:59:23]configsip: 0, SPIWP:0xee
[11:59:23]clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
[11:59:23]mode:DIO, clock div:2
[11:59:23]load:0x3fff0018,len:4
[11:59:23]load:0x3fff001c,len:1044
[11:59:23]load:0x40078000,len:8896
[11:59:23]load:0x40080400,len:5828
[11:59:23]entry 0x400806ac
[11:59:23][I][logger:166]: Log initialized
[11:59:23][C][ota:366]: There have been 8 suspected unsuccessful boot attempts.
[11:59:23][I][app:029]: Running through setup()...
[11:59:23][C][power_supply:010]: Setting up Power Supply...
[11:59:23][C][light:097]: Setting up light 'unused_yeelight night light'...
[11:59:23][D][light:265]: 'unused_yeelight night light' Setting:
[11:59:23][D][light:278]:   Brightness: 100%
[11:59:23][C][light:097]: Setting up light 'unused_yeelight ceiling light'...
[11:59:23][D][light:265]: 'unused_yeelight ceiling light' Setting:
[11:59:23][D][light:278]:   Brightness: 100%
[11:59:23][D][light:282]:   Color Temperature: 1.0 mireds
[11:59:23][D][light:287]:   Red=100%, Green=100%, Blue=100%
[11:59:24][D][esp32_ble_tracker:148]: Starting scan...
[11:59:24][D][binary_sensor:034]: 'unused_yeelight Status': Sending initial state OFF
[11:59:24][C][adc:018]: Setting up ADC 'unused_yeelight power supply'...
[11:59:24][D][text_sensor:015]: 'unused_yeelight Esphome Version': Sending state '1.15.3 Feb 25 2021, 11:57:31'
[11:59:24][C][wifi:033]: Setting up WiFi...
[11:59:24][I][wifi:194]: WiFi Connecting to 'xxxxxxxx'...
[11:59:24][D][sensor:092]: 'unused_yeelight Uptime': Sending state 0.00001 days with 0 decimals of accuracy
[11:59:24][D][adc:056]: 'unused_yeelight power supply': Got voltage=3.90V
[11:59:24][D][sensor:092]: 'unused_yeelight power supply': Sending state 3.90000 V with 2 decimals of accuracy
[11:59:24][D][esp32_ble_tracker:544]: Found device 50:XX:XX:XX:XX:XX RSSI=-62
[11:59:24][D][esp32_ble_tracker:565]:   Address Type: PUBLIC
[11:59:24][D][esp32_ble_tracker:544]: Found device 49:XX:XX:XX:XX:XX RSSI=-91
[11:59:24][D][esp32_ble_tracker:565]:   Address Type: RANDOM
[11:59:29]E (6637) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
[11:59:29]E (6637) task_wdt:  - IDLE0 (CPU 0)
[11:59:29]E (6637) task_wdt: Tasks currently running:
[11:59:29]E (6637) task_wdt: CPU 0: loopTask
[11:59:29]E (6637) task_wdt: Aborting.
[11:59:29]abort() was called at PC 0x400ea723 on core 0
[11:59:29]
[11:59:29]Backtrace: 0x40091698:0x3ffbe520 0x400918a9:0x3ffbe540 0x400ea723:0x3ffbe560 0x4008494e:0x3ffbe580 0x4000bfed:0x3ffcdb20 0x4008f135:0x3ffcdb30 0x4008e28f:0x3ffcdb50 0x400d8df1:0x3ffcdb90 0x401df9c9:0x3ffcdc30 0x401dfa95:0x3ffcdc50 0x400e0959:0x3ffcdc70 0x400e3632:0x3ffcdcc0 0x400e8973:0x3ffcdf50 0x4008e535:0x3ffcdf70
WARNING Found stack trace! Trying to decode it
WARNING Decoded 0x40091698: invoke_abort at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/esp32/panic.c:707
WARNING Decoded 0x400918a9: abort at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/esp32/panic.c:707
WARNING Decoded 0x400ea723: task_wdt_isr at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/esp32/task_wdt.c:252
WARNING Decoded 0x4008494e: _xt_lowint1 at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/freertos/xtensa_vectors.S:1154
WARNING Decoded 0x4008f135: vTaskExitCritical at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/freertos/tasks.c:3507
WARNING Decoded 0x4008e28f: xQueueGenericReceive at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/freertos/queue.c:2520
WARNING Decoded 0x400d8df1: esphome::esp32_ble_tracker::ESP32BLETracker::loop() at /home/sebastian/esphome/unused_yeelight/src/esphome/components/esp32_ble_tracker/esp32_ble_tracker.cpp:241
WARNING Decoded 0x401df9c9: esphome::Component::call_loop() at /home/sebastian/esphome/unused_yeelight/src/esphome/core/component.cpp:111
WARNING Decoded 0x401dfa95: esphome::Component::call() at /home/sebastian/esphome/unused_yeelight/src/esphome/core/component.cpp:111
WARNING Decoded 0x400e0959: esphome::Application::setup() at /home/sebastian/esphome/unused_yeelight/src/esphome/core/application.cpp:50 (discriminator 2)
WARNING Decoded 0x400e3632: setup() at /home/sebastian/esphome/unused_yeelight/src/main.cpp:360
WARNING Decoded 0x400e8973: loopTask(void*) at /home/sebastian/.platformio/packages/framework-arduinoespressif32@src-ff59aeb8b43c3669326fe991d70309ba/cores/esp32/main.cpp:14
WARNING Decoded 0x4008e535: vPortTaskWrapper at /home/paul/src/esp32-arduino-lib-builder/esp-idf/components/freertos/port.c:403
[11:59:29]
[11:59:29]Rebooting...
```
</details>


## GPIOs

### yeelink.light.ceil26

| Name                | Label  | ESP32 GPIO   | Stock firmware limits |
| ------------------- | ------ | ------------ |-----------------------|
| Warm white PWM      | W      | GPIO19       |           0.4V - 1.5V |
| Cold white PWM      | C      | GPIO21       |           0.5V - 3.0V |
| Night light PWM     | NL     | GPIO23       |                       |
| Power supply GPIO   | STB    | GPIO23       |                       |
| VCC measurement     | ADC1   | GPIO35       |                       |
| TXD                 |        | GPIO1        |                       |
| RXD                 |        | GPIO3        |                       |
| GPIO0               | TP3    | GPIO0        |                       |

The ESP32 will enter the serial bootloader when GPIO0 (test point TP3) is held low (GND) on reset / power.

### yeelink.light.ceiling10

| Name                | Label  | ESP32 GPIO   | Stock firmware limits |
| ------------------- | ------ | ------------ |-----------------------|
| Warm white PWM      | W      | GPIO19       |         0.42V - 2.55V |
| Cold white PWM      | C      | GPIO21       |         0.41V - 2.51V |
| Night light PWM     | NL     | GPIO23       |                   N/A |
| Red PWM             | R      | GPIO33       |                       |
| Green PWM           | G      | GPIO26       |                       |
| Blue PWM            | B      | GPIO27       |                       |
| Power supply GPIO   | ON/OFF | GPIO22       |            3.08V / 0V |
| TXD                 | TX     | GPIO1        |                       |
| RXD                 | RX     | GPIO3        |                       |
| GPIO0               | IO0    | GPIO0        |                       |

The ESP32 will enter the serial bootloader when GPIO0 (test point IO0 at the back) is held low (GND) on reset / power.


### yeelink.light.ceiling15

| Name                | Label  | ESP32 GPIO   | Stock firmware limits |
| ------------------- | ------ | ------------ |-----------------------|
| Warm white PWM      | W      | GPIO19       |         0.67V - 3.08V |
| Cold white PWM      | C      | GPIO21       |         0.67V - 3.08V |
| Night light PWM     | NL     | GPIO23       |          0.06V - 3.0V |
| VCC measurement     | ADC1   | GPIO36       |                  2.0V |
| VCC measurement     | ADC2   | GPIO32       |                 1.76V |
| TXD                 |        | GPIO1        |                       |
| RXD                 |        | GPIO3        |                       |
| GPIO0               | TP1    | GPIO0        |                       |

The ESP32 will enter the serial bootloader when GPIO0 (test point TP1 at the back) is held low (GND) on reset / power.

```
  ADC1 NL  WW GND           O  -- GND
   |   |   |   |            O  -- TXD
   8   6   4   2            O  -- RXD
   |   |   |   |           [O] -- 3V3
   o   o   o   o

   o   o   o   o
   |   |   |   |
   7   5   3   1
   |   |   |   |
  3V3 ADC2 CW GND
```

### yeelink.light.ceiling22

| Name                | Label  | ESP32 GPIO   | Stock firmware limits |
| ------------------- | ------ | ------------ |-----------------------|
| Warm white PWM      | W      | GPIO19       | 14% - 50% duty cycle  |
| Cold white PWM      | C      | GPIO21       | 20% - 88% duty cycle  |
| Night light PWM     | NL     | GPIO23       | 10% - 100% duty cycle |
| VCC measurement     | ADC1   | GPIO36       |                       |
| 3.3V                | TP1    |              |                       |
| GND                 | TP2    |              |                       |
| GPIO0               | TP3    | GPIO0        |                       |
| TXD                 | TP4    | GPIO1        |                       |
| RXD                 | TP5    | GPIO3        |                       |

The ESP32 will enter the serial bootloader when GPIO0 (test point TP3 at the back) is held low (GND) on reset / power.

### yeelink.light.ceiling20

| Name                | Label  | ESP32 GPIO   | Stock firmware limits |
| ------------------- | ------ | ------------ |-----------------------|
| 3.3V                | 3.3    | 3.3V         |                       |
| GND                 | GND    | GND          |                       |
| Red PWM             | R      | GPIO33       |                       |
| Green PWM           | G      | GPIO26       |                       |
| Blue PWM            | B      | GPIO27       |                       |
| Power supply GPIO   | STA    | GPIO22       |             3.3V / 0V |
| Cold white PWM      | C      | GPIO21       |                 3.16V |
| Warm white PWM      | W      | GPIO19       |                 1.58V |
| TXD                 | TP5    | GPIO1        |                       |
| RXD                 | TP4    | GPIO3        |                       |
| GPIO0               | TP3    | GPIO0        |                       |


The ESP32 will enter the serial bootloader when GPIO0 (test point IO0 at the back) is held low (GND) on reset / power.

### yeelink.light.lamp9, yeelink.light.lampUNKNOWN

| Name                | Label           | ESP32 GPIO   | Stock firmware limits |
| ------------------- | --------------- | ------------ |-----------------------|
| Warm white PWM      |                 | GPIO2        |        0.226V - 3.28V |
| Cold white PWM      |                 | GPIO27       |        0.194V - 3.28V |
| Night light PWM     |                 | GPIO12       |        0.197V - 3.27V |
| Power supply GPIO   |                 | GPIO4        |                       |
| Button              |                 | GPIO14       |                       |
| TXD                 | TX, TP9         | GPIO1        |                       |
| RXD                 | RX, TP10        | GPIO3        |                       |
| GPIO0               | TP next to C37  | GPIO0        |                       |
| SCL (I2C)           |                 | GPIO18       |                       |
| SDA (I2C)           |                 | GPIO17       |                       |
| LED (YLCT03YL only) |                 | GPIO33       |                       |
| 3.3V                | +, TP7          | 3.3V         |                       |
| GND                 | -, TP8          | GND          |                       |


## Disassembly

### YLXD76YL

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/012.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/012.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/011.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/011.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/007.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/010.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/010.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/008.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd76yl/thumbnails/008.jpg" width="18%">
</a>


### YLXD41YL

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd41yl/thumbnails/007.jpg" width="18%">
</a>


### C2001C550

Remove the single safety screw on the back and rotate the cover counterclockwise.

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/007.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/008.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/008.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/009.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/009.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/010.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/010.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/011.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/c2001c550/thumbnails/011.jpg" width="18%">
</a>


### YLDL01YL

The grey plastic frame can easily be removed with a credit card between the outer white body and the grey frame. It is just clipped into place. Kudos to @fhloston.

References, sources & inspiration:

  * https://templates.blakadder.com/yeelight_YLDL01YL.html
  * https://photos.app.goo.gl/T3F4aaXuYgvFXRmi7
  * https://github.com/arendst/Tasmota/discussions/10762

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/007.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/008.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/yldl01yl/thumbnails/008.jpg" width="18%">
</a>


### YLXD42YL

Rotate the cover counterclockwise.

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/006.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/007.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd42yl/thumbnails/007.jpg" width="18%">
</a>

### YLCT02YL

Remove the 4 screws under the foot. The lower shell is held by 8 clips and can be separated with a plastic card and some force.

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylct02yl/thumbnails/004.jpg" width="18%">
</a>

### YLXD50YL

Remove the cover counterclockwise, disconnect from the AC (be safe!), pull out the RGB - C - W connectors (mark one of the last two). Unscrew the 2 screws from the base, and you can remove the controller box.
Pick a flat screwdriver/plastic card and remove the plastic base from the box. Sometimes its harder, because the thermalpad. Pull out the PCB from the box, and you can remove the ESP module. Maybe some cleaning required from the glue.
The test points are in the back of the ESP's PCB.

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/001.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/001.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/002.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/002.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/003.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/003.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/004.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/004.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/005.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/005.jpg" width="18%">
</a>

<a href="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylylxd50yl/006.jpg" target="_blank">
<img src="https://raw.githubusercontent.com/syssi/esphome-yeelight-ceiling-light/main/images/ylxd50yl/thumbnails/006.jpg" width="18%">
</a>
