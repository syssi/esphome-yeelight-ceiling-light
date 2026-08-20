# Stock firmware catalogue (`yeelink.light.*`)

Metadata for the current stock firmware of every Yeelight lighting model that
Xiaomi's cloud will describe. Collected with `tools/cloud_fw_info.py`; see
`docs/xiaomi-cloud-firmware.md` for how and for the endpoint's quirks.

Change descriptions are reproduced **verbatim** as Xiaomi publishes them, in
whatever language they were written. Some are Chinese, some English, many empty.

No firmware images are included here - only their sizes and checksums, so a copy
you fetch yourself can be verified. `tools/cloud_fw_info.py --model <model>
--download <dir>` fetches one, and checks the md5 against the value the endpoint
reported before writing the file.

Chip is read from the image header, not guessed: ESP32 images carry a `chip_id`,
ESP8266 images share the `0xE9` magic but have no extended header and are told
apart by load address.

120 models. `no image` means the endpoint answered but offered no firmware.

| Model | Product | Chip | Version | Size | MD5 | Published | Change description (verbatim) |
| ----- | ------- | ---- | ------- | ---- | --- | --------- | ----------------------------- |
| `yeelink.light.bslamp1` | 米家床头灯 | - | 1.6.6_0172 | 661555 | 6dc10803e4922431078b4db52d8b6aaa | 2019-08-01 |  |
| `yeelink.light.bslamp2` | 米家床头灯2代 | ESP32 | 2.1.7_0047 | 1502820 | 02a3da7c4147c0ee781735e054bdf6f8 | 2022-03-03 | 1.optimization for net; |
| `yeelink.light.bslamp3` | Yeelight 床头灯2代 | ESP32 | 2.1.7_0033 | 1529876 | 0b329a753b9bb69043c968ec5d88275e | 2022-03-08 | Improve the HomeKit experience |
| `yeelink.light.ceil26` | Yeelight 纤玉吸顶灯 C2001 | ESP32 | 2.0.6_0010 | 1570580 | 39296407f463cebc1dc5cb4d05bd5aa9 | 2020-07-15 |  |
| `yeelink.light.ceil27` | Yeelight 智能客厅吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil29` | Yeelight 初心吸顶灯 A2001 | ESP32 | 2.0.6_0002 | 1572644 | 09bb9a9549e3ecbc75edb99e4c14d210 | 2020-07-15 |  |
| `yeelink.light.ceil30` | Yeelight LED智能吊灯 | ESP32 | 2.0.6_0015 | 1218276 | 8f849d6a004986a2882347771e5b549b | 2023-01-31 | 1. Optimize network stability. |
| `yeelink.light.ceil31` | Yeelight 智能吸顶灯 客厅款 | ESP32 | 2.0.6_0006 | 1307508 | 208b3f233d3fc3c660d1db52d30b9aef | 2020-07-15 |  |
| `yeelink.light.ceil32` | Yeelight LED智能吸顶灯 | ESP32 | 2.0.6_0009 | 1329140 | d863c94e9e716771a8f6a65af7a7debe | 2020-07-15 |  |
| `yeelink.light.ceil33` | Yeelight 儿童吸顶灯 C2002 | - | - | - | - | - |  |
| `yeelink.light.ceil34` | Mi Smart LED Ceiling Light (350mm) | ESP32 | 2.1.7_0025 | 1540276 | 17a958a414a83d301c95970859758b22 | 2024-11-12 | Adapt to Xiaomi Smart Ceiling Light remote control. |
| `yeelink.light.ceil35` | Yeelight 智能LED吊灯 | - | - | - | - | - |  |
| `yeelink.light.ceil36` | Yeelight 智能LED吊灯S | - | - | - | - | - |  |
| `yeelink.light.ceil38` | Yeelight 智能吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil39` | Yeelight 智能吸顶灯升级版 | - | - | - | - | - |  |
| `yeelink.light.ceil40` | Yeelight智能LED吸顶灯升级版 | - | - | - | - | - |  |
| `yeelink.light.ceil42` | Yeelight 智能彩光吊灯 | - | - | - | - | - |  |
| `yeelink.light.ceil43` | Yeelight Arwen Ceiling Light D | ESP32 | 2.1.7_0018 | 1585364 | 3a7e92b26c0952cc433414ec0780f8f6 | 2024-12-04 | 1.Resolve the issue of Google Home preset color adaptation. / 2.Added 12 new lighting effects. |
| `yeelink.light.ceil45` | Yeelight 超薄智能吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil46` | Yeelight 凌动LED吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil47` | Yeelight 超薄智能吸顶灯PRO | - | - | - | - | - |  |
| `yeelink.light.ceil49` | Yeelight RGB Smart Ceiling Light | ESP32 | 2.2.9_0020 | 1414692 | 3f0cc8b4ba53cc115dca3d45bf8943e6 | 2025-06-10 | Ver.20 |
| `yeelink.light.ceil55` | Yeelight智能吸顶灯 氛围客厅款 | - | - | - | - | - |  |
| `yeelink.light.ceil56` | Yeelight智能吸顶灯 氛围卧室款 | - | - | - | - | - |  |
| `yeelink.light.ceil58` | Yeelight 极月吊灯 L1200 | - | - | - | - | - |  |
| `yeelink.light.ceil59` | Yeelight 吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil60` | Yeelight 智能色温吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil63` | Yeelight 智能超薄吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceil64` | Yeelight 漫反射吸顶灯 | - | - | - | - | - |  |
| `yeelink.light.ceila` | Yeelight 纤玉吸顶灯C2001 | ESP32 | 2.1.7_0011 | 1538484 | 673f6281f6ff57f0f849fa95ed162700 | 2021-05-31 | 1 Fix 0 bright will showed when light close by alexa. |
| `yeelink.light.ceilb` | Yeelight 氛围吸顶灯升级版 | ESP32 | 2.1.7_0011 | 1529508 | 62c11cc730dfb6f9678259a4158c8acd | 2021-05-27 | 1.Improve network connection stability.  / |
| `yeelink.light.ceilc` | Yeelight 氛围吸顶灯 | ESP32 | 2.0.6_0005 | 1264676 | 33f796360c7133206c8da92e0f1cf767 | 2020-12-15 |  |
| `yeelink.light.ceild` | Yeelight Minas Ceiling Light | - | - | - | - | - |  |
| `yeelink.light.ceile` | Yeelight LED吸顶灯升级版 | ESP32 | 2.1.7_0004 | 1546356 | 61c0728bfe098240a14e28cdad3dee1a | 2022-03-07 | Solve the problem of occasional failure of firmware upgrade |
| `yeelink.light.ceiling1` | Yeelight LED吸顶灯 | - | 1.5.9_0189 | 650224 | 0324c98a9bfde8faf531ab9014231d99 | 2019-10-16 |  |
| `yeelink.light.ceiling10` | Yeelight 皓石 LED吊灯 | ESP32 | 2.0.6_0049 | 1323604 | df39251190b94c50c518d0cbd10523f7 | 2020-06-12 |  |
| `yeelink.light.ceiling11` | Yeelight LED吸顶灯（升级版） | ESP32 | 2.0.6_0019 | 1537524 | 2a6a4b717dfaf1b5d37264ac3798405f | 2019-10-16 |  |
| `yeelink.light.ceiling12` | Yeelight LED吸顶灯 Pro | ESP32 | 2.0.6_0041 | 1334036 | 0c105616440a79a51d3505e3fad89ba7 | 2020-04-16 |  |
| `yeelink.light.ceiling13` | Yeelight LED吸顶灯 | ESP32 | 2.0.6_0019 | 1324548 | 0007e8cceeadf34e7c871c46d6772f6c | 2020-07-15 | optimize wifi compatibility |
| `yeelink.light.ceiling14` | Yeelight LED吸顶灯 Mini | ESP32 | 2.0.6_0016 | 1311972 | ab643f3d0388c135cf1e6aa90836fa0b | 2019-10-16 |  |
| `yeelink.light.ceiling15` | Yeelight 皎月 LED吸顶灯480（升级版） | ESP32 | 2.0.6_0023 | 1563604 | bcc844683aab8550646a6b97c8662465 | 2020-07-15 | optimize wifi compatibility |
| `yeelink.light.ceiling16` |  Yeelight 星宇 LED吸顶灯 | ESP32 | 1.3.2_0005 | 1094820 | fde02e55ddb68c33ad743214b83de975 | 2019-10-16 |  |
| `yeelink.light.ceiling17` | Yeelight 智能吸顶灯 卧室款 | ESP32 | 1.3.2_0005 | 1093204 | c22827f1710392edb20ec8e14b66ae6d | 2019-10-16 |  |
| `yeelink.light.ceiling18` | Yeelight LED吸顶灯 Pro | ESP32 | 2.0.6_0009 | 1298452 | fb7db24de38611e813b4d6e694302282 | 2019-10-16 |  |
| `yeelink.light.ceiling19` | Yeelight RGB彩光吸顶灯 客厅款 | ESP32 | 2.0.6_0019 | 1553444 | b52f8f45bd9941ba49473d7a9afc64bc | 2019-10-28 |  |
| `yeelink.light.ceiling2` | Yeelight LED吸顶灯青春版 | - | 1.5.9_0034 | 649064 | 4280022a6847c0c7a455b196f6cecdc1 | 2019-06-27 |  |
| `yeelink.light.ceiling20` | Yeelight RGB彩光吸顶灯 卧室款 | ESP32 | 2.0.6_0016 | 1552260 | c9197273fbfa88a1428c98639b46d091 | 2019-10-16 |  |
| `yeelink.light.ceiling21` | 米家客厅吸顶灯 | ESP32 | 2.0.6_0025 | 1541572 | 9fd9c051c444c6ea2cf8beca1df67215 | 2019-11-18 |  |
| `yeelink.light.ceiling22` | 米家卧室吸顶灯450 | ESP32 | 2.1.7_0042 | 1526500 | b0c794f9d148407ee0a20a3d780c3016 | 2024-11-05 | Adapt to Xiaomi Smart Ceiling Light remote control. |
| `yeelink.light.ceiling23` | 米家卧室吸顶灯350 | ESP32 | 2.0.6_0025 | 1541556 | 935ee634ff2c58d9f3f6cdf70f76dc6d | 2019-11-18 |  |
| `yeelink.light.ceiling24` | Yeelight 皎月 LED吸顶灯 260 | ESP32 | 1.3.2_0003 | 1094820 | 204896cfbdbf3be25dea27a60a5f307c | 2019-10-16 |  |
| `yeelink.light.ceiling3` | Yeelight 皎月LED吸顶灯 | - | 2.0.2_0048 | 726978 | 13b7471a0f84e3cfbac78c116fae7a1c | 2020-06-29 | Optimize wifi compatibility |
| `yeelink.light.ceiling4` | Yeelight 皎月LED吸顶灯 | - | 2.0.2_0056 | 735529 | fce79dfe37da4ecc4d14d7c8156d17e3 | 2020-06-29 |  |
| `yeelink.light.ceiling5` | 米家LED吸顶灯 | - | 2.0.6_0027 | 751169 | ecdd42fa0d090b23750b9e82c4287029 | 2021-03-03 | 1. Add light off gradient support / 2. Add close switch shortcut key option |
| `yeelink.light.ceiling6` | Yeelight 皓石LED吸顶灯 Pro | - | 2.0.2_0026 | 730412 | 60457cae98cb615b2aa1e88b97368427 | 2020-06-29 |  |
| `yeelink.light.ceiling7` | Yeelight 皓石LED吸顶灯 | ESP32 | 2.0.6_0047 | 1310692 | 4ec3fcf264bee827ff013e5641f025b9 | 2019-10-16 |  |
| `yeelink.light.ceiling8` | Yeelight 皓石LED吸顶灯 Plus | ESP32 | 2.0.6_0044 | 1321636 | 0dc1455ddbe3d91ad954f1a41afe3f9d | 2020-06-29 | optimize wifi compatibility |
| `yeelink.light.ceiling9` | Yeelight 皓石LED吸顶灯 Pro | ESP32 | 1.3.2_0021 | 1096308 | f3698f031e7031c884f3f1db8ebd7a13 | 2019-06-27 |  |
| `yeelink.light.color1` | Yeelight 彩光灯泡 | - | 1.4.2_0076 | 317812 | 5c805f67a6040aa655f139d256d1f378 | 2019-08-12 |  |
| `yeelink.light.color2` | Yeelight LED灯泡（彩光版） | - | 2.0.6_0065 | 598012 | 587a8b32d5690cb99d62a02e4aedd179 | 2019-11-01 | network stability enhancements |
| `yeelink.light.color3` | Mi LED Smart Bulb (White and Color) | - | 2.0.6_0035 | 594324 | 448ebed6d8dfbe66eb02a5f0de03a544 | 2019-10-31 | network stability enhancements |
| `yeelink.light.color4` | Yeelight LED灯泡1S（彩光版） | ESP32 | 2.1.7_0039 | 1502532 | f13ec12c9d53573748054b36a709e75a | 2022-01-06 | Optimize HomeKit experience. |
| `yeelink.light.color5` | Mi Smart LED Bulb Essential (White and Color) | ESP8266 | 2.0.8_0022 | 710040 | 1e7a0261a2e2f01b675da0caac3bccbc | 2021-08-31 | Fixed light blink problem when device reboot . |
| `yeelink.light.color7` | Mi LED Smart Color Bulb (B22) | - | - | - | - | - |  |
| `yeelink.light.color8` | Yeelight LED灯泡1S 彩光版 | - | - | - | - | - |  |
| `yeelink.light.colora` | Yeelight 彩光灯泡1SE | ESP8266 | 2.0.8_0009 | 661092 | 8fc5e35ea0381d254de3c4d311b4d5bf | 2021-06-03 | 1. Increase the brightness under white light. |
| `yeelink.light.colorb` | Yeelight LED灯泡W3（彩光版） | ESP8266 | 2.0.8_0010 | 660884 | e4db3bb856c200f921c4428d35dc0e33 | 2021-04-20 | Fixed known problem. |
| `yeelink.light.colorc` | Yeelight GU10 smart bulb W1（multicolor） | ESP8266 | 2.0.8_0016 | 669060 | 4007b28bbc69548bf42789408fd2a9fb | 2020-12-29 | Optimize color light mixing parameters to improve user dimming experience |
| `yeelink.light.colore` | Yeelight Smart LED Bulb W4 lite（multicolor） | ESP32 | 2.1.7_0009 | 1538340 | 0739ab536055d27425cfa919fb8e2574 | 2022-08-09 | 1. Optimize experience. |
| `yeelink.light.ct2` | Yeelight LED灯泡（色温版） | - | 2.0.6_0041 | 583436 | fe1851498deb363e128bd7ff1cba3c28 | 2019-10-23 | network stability enhancements |
| `yeelink.light.cta` | Yeelight LED灯泡W3（色温版） | ESP8266 | 2.0.8_0007 | 640820 | 83a1f0991e814ad6fc85328095f27c7e | 2020-12-02 | 1. Optimize network stability. / 2. Fix known problem. |
| `yeelink.light.ctc` | Yeelight Smart LED Bulb W4 lite（dimmable） | ESP32 | 2.1.7_0005 | 1520884 | a606aceaa9964478ac4d6d2f38025fe1 | 2022-06-15 | 1.support homekit soft auth; / 2. disable bt after net config success; |
| `yeelink.light.fancl1` | Yeelight 逸扬风扇吊灯（智能款） | - | - | - | - | - |  |
| `yeelink.light.fancl2` | Yeelight  烁影直流变频风扇灯 S2001 | - | - | - | - | - |  |
| `yeelink.light.fancl5` | Yeelight 逸扬直流变频风扇吊灯 C900 | - | - | - | - | - |  |
| `yeelink.light.fancl6` | Yeelight 逸扬直流变频风扇吊灯 C1060 | ESP32 | 2.0.6_0009 | 1262932 | 7e892d0c2e7fc7aa45fb64718f4fc4ed | 2021-06-15 | 1  Modify voltage threshold according to new hardware. |
| `yeelink.light.fancl8` | Yeelight 智能风扇灯 | - | - | - | - | - |  |
| `yeelink.light.fancl9` | Yeelight Bladeless turbo fan Light | - | - | - | - | - | Ver.3 |
| `yeelink.light.lamp1` | 米家台灯 | - | 1.3.9_0062 | 485320 | f7862f050f830ca08b327a57249607ac | 2019-12-30 |  |
| `yeelink.light.lamp10` | Yeelight 星辰落地灯 | ESP32 | 2.0.6_0013 | 1651796 | 3a45f8a1a3403143d735149974454850 | 2019-10-16 |  |
| `yeelink.light.lamp15` | Yeelight 显示器挂灯 | ESP8266 | 2.0.8_0038 | 653508 | 81b7ad62d2db01eb23cafdb40825ae00 | 2022-06-06 | Add ambient lighting related functions to smart scenes |
| `yeelink.light.lamp17` | Yeelight 无线充电台灯 | - | - | - | - | - |  |
| `yeelink.light.lamp2` | 米家台灯Pro | ESP32 | 2.1.7_0046 | 1480628 | e4da24cf5939468d8e237357b085d628 | 2022-03-01 | 1.optimization for net; |
| `yeelink.light.lamp22` | 米家智能显示器挂灯1S | - | - | - | - | - |  |
| `yeelink.light.lamp27` | 米家台灯1S 增强版 | - | - | - | - | - |  |
| `yeelink.light.lamp28` | 米家台灯1S增强版 耀夜黑 | - | - | - | - | - |  |
| `yeelink.light.lamp3` | Yeelight 智能护眼台灯 | - | 1.3.9_0024 | 487512 | aaf3c05c98db26f9e21f7680670b01c8 | 2019-10-16 |  |
| `yeelink.light.lamp30` | Yeelight 立式学习灯 V8 | - | - | - | - | - |  |
| `yeelink.light.lamp34` | Yeelight 立式学习灯 V6 | - | - | - | - | - |  |
| `yeelink.light.lamp38` | Yeelight 桌面学习灯 A8 Pro | - | - | - | - | - |  |
| `yeelink.light.lamp39` | Yeelight 显示器挂灯 Ultra | - | - | - | - | - |  |
| `yeelink.light.lamp4` | 米家台灯 1S | ESP32 | 2.1.7_0020 | 1481044 | e4787e9e9605c9109d04799244b82ccf | 2022-03-14 | 1.bug fix; |
| `yeelink.light.lamp41` | Yeelight 立式学习灯 V5 | - | - | - | - | - |  |
| `yeelink.light.lamp42` | Yeelight 显示器挂灯2 Pro | - | - | - | - | - |  |
| `yeelink.light.lamp43` | Yeelight Tube Plus2屏幕挂灯 | - | - | - | - | - |  |
| `yeelink.light.lamp5` | Yeelight 智能护眼台灯 Prime | - | 1.3.9_0014 | 486872 | d756df90bf787d18e37a6aba898b5716 | 2019-10-16 |  |
| `yeelink.light.lamp7` | Yeelight LED光感台灯V1 | ESP32 | 2.0.6_0027 | 1531764 | cb94bc9b3cfb67ee6f79adcb1c77b965 | 2019-10-16 |  |
| `yeelink.light.lamp9` | Yeelight 星辰LED台灯 | ESP32 | 2.1.7_0031 | 1494084 | 08f09790930817d39d02cb2d5dac1840 | 2022-01-11 | Fix uncontrollable problems in homekit |
| `yeelink.light.mono1` | Yeelight 白光灯泡 | - | 1.4.2_0056 | 303604 | 61789affb6ddd8ae4bee3a50a5bed8cb | 2018-07-13 | fix known issues |
| `yeelink.light.mono10` | Yeelight P 系列青空灯 3060 | - | - | - | - | - |  |
| `yeelink.light.mono12` | Yeelight 青空灯 G6 Pro | - | - | - | - | - |  |
| `yeelink.light.mono4` | Yeelight LED 灯泡1S（白光版） | ESP32 | 1.3.2_0007 | 1172996 | cb12f53891fd7c363cec5b76144a9de6 | 2019-10-16 |  |
| `yeelink.light.mono5` | Yeelight LED 灯丝灯 | ESP32 | 1.3.2_0006 | 1171844 | 6a326fd4d592653f5d50c029f2f8b3ef | 2019-10-16 |  |
| `yeelink.light.mono6` | Mi Smart LED Bulb | ESP8266 | 2.0.8_0015 | 647988 | 11cce94fc09c676b3ee8ea2b8b5255c2 | 2020-05-05 | Fix Alexa control issue. |
| `yeelink.light.mono7` | Yeelight 青空灯 | - | - | - | - | - |  |
| `yeelink.light.mono8` | Yeelight 青空灯G6 | - | - | - | - | - |  |
| `yeelink.light.mono9` | Yeelight 青空面板灯A10 | - | - | - | - | - |  |
| `yeelink.light.monoa` | Yeelight LED smart bulb W3(dimmable) | ESP8266 | 2.0.8_0009 | 640596 | e0132c15e5a17f5e456c6c7fa1cb1243 | 2021-06-11 | 1. Optimize network stability. |
| `yeelink.light.monob` | Yeelight GU10 Smart Bulb W1(dimmable) | ESP8266 | 2.0.8_0009 | 649492 | cac3c518671e20708b1de93b66758004 | 2020-11-09 | Optimizing bulb power |
| `yeelink.light.panel1` | Yeelight 皓白 LED面板灯 | ESP32 | 2.0.6_0021 | 1295572 | 4e5e4ba8fe09b81cdb045fdaa9356bf3 | 2019-10-16 |  |
| `yeelink.light.panel3` | Yeelight 皓白 LED面板灯 Pro | ESP32 | 1.3.2_0005 | 1095780 | 5a6746e401fb651f523971db5dd66248 | 2019-10-16 |  |
| `yeelink.light.panel7` | Yeelight人体存在面板灯 | - | - | - | - | - |  |
| `yeelink.light.plate2` | Yeelight 智能奇光板 | ESP32 | 2.1.7_0016 | 1316148 | 61bd98513b3c8dd31550562d70ef5680 | 2021-05-25 | 1  Fix can not restore bright and RGB value issue. |
| `yeelink.light.strip1` | Yeelight彩光灯带 | - | 1.4.2_0050 | 316228 | 26a73c0a2bd96ec71b818912dd8ccd93 | 2018-11-05 | increase color saturation |
| `yeelink.light.strip2` | Yeelight彩光灯带（延长版） | - | 2.0.6_0073 | 594684 | 3918293a729422b2b760a28c1b51b48b | 2019-10-23 | network stability enhancements |
| `yeelink.light.strip4` | Yeelight 泛影LED灯带 | ESP32 | 2.0.6_0011 | 1288644 | 9e6fc9f4f6991a0fe58906f0c26054f3 | 2019-10-28 |  |
| `yeelink.light.strip6` | Yeelight LED 彩光灯带1S | ESP32 | 2.1.7_0020 | 1511172 | e39b440d1b2acf847a4efde76249b47e | 2022-03-29 | fix homekit issue |
| `yeelink.light.strip8` | Yeelight Chameleon 幻彩灯带 | ESP32 | 2.1.7_0021 | 1520612 | a6a22d7970ec5b7100f929dec198392f | 2022-06-08 | Improve network stability |
| `yeelink.light.stripa` | Yeelight LED 彩光灯带 1S | - | - | - | - | - |  |
| `yeelink.light.stripb` | Xiaomi Smart Lightstrip | ESP32 | 2.1.7_0031 | 1504388 | 277de72c0884c40f73c25d2b9a7de09d | 2022-08-17 | 1 / Fix known issues |

## Version history

Available only for models with a device on the account used to query - the
history endpoint is keyed on a device id, not a model. Descriptions verbatim.

Older versions are listed but **cannot be downloaded**: the endpoint returns the
current build whatever version is requested. See the docs for the test.

### `yeelink.light.ceilb`

| Version | Published | Description (verbatim) |
| ------- | --------- | ---------------------- |
| 2.1.7_0011 | - | 1.Improve network connection stability.  / |
| 2.0.6_0008 | 2020-12-18 | Solve the problem of incorrect configuration of the gradient value of the remote control and some smart options |

### `yeelink.light.ceiling10`

| Version | Published | Description (verbatim) |
| ------- | --------- | ---------------------- |
| 2.0.6_0049 | - | 优化wifi兼容性 / 开灯默认为渐变效果 |
| 2.0.6_0042 | 2019-10-16 | 优化小夜灯功能 |
| 1.3.2_0028 | 2019-06-27 | 修复某些路由器信号干扰问题。 |

### `yeelink.light.lamp9`

| Version | Published | Description (verbatim) |
| ------- | --------- | ---------------------- |
| 2.1.7_0031 | - | Fix uncontrollable problems in homekit |
| 2.1.7_0029 | 2021-12-23 | Fix known issues. |
| 2.0.6_0017 | 2019-10-16 | 1. 修复已知问题 / 2. 提升网络稳定性和兼容性 |

