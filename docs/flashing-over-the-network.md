# Flashing over the network, without opening the device

Every documented conversion in this project flashes over UART, which means taking
the device apart. On ESP32 Yeelights running stock firmware there is another
route: the device's own `miIO.ota` update mechanism can be pointed at a file on
your LAN.

Confirmed here on `yeelink.light.lamp9` firmware `2.1.7_0031`
(`miio_ver 0.0.9`). Third parties report the same mechanism working on
`yeelink.light.ceiling22`; that was not verified as part of this work.

> **Read the limits first.** This writes only the application partition. The
> stock bootloader and partition table remain, so it cannot recover a device that
> will not boot - that still needs UART. Take a flash backup over UART first if
> you want any way back, because no stock firmware image for these models is
> archived anywhere public.

---

## Why the obvious approach fails

Sending `miIO.ota` **locally** over UDP 54321 with a valid token is refused:

```
{"code": -30020, "message": "service not available."}
```

The response is identical with no parameters at all and with a full payload,
which places the refusal above the parameter layer: five payload shapes were
tried and none made any difference. Local OTA is disabled on this firmware, and
the update command has to arrive from the Xiaomi cloud instead.

Useful error signatures when probing:

| Response | Meaning |
| -------- | ------- |
| `-9999 user ack timeout` | method not implemented; firmware never acknowledges |
| `-32602 Invalid param.` | method exists, parameter validation rejected the input |
| `-30020 service not available.` | method exists, the service refuses before parsing |

## The route that works

Xiaomi's cloud exposes an RPC relay that forwards a raw miIO call to a device you
own. Because the command then originates from the server, the OTA service accepts
it - and `app_url` may point anywhere the device can reach, including a private
LAN address over plain HTTP.

```
POST https://<region>.api.io.mi.com/app/home/rpc/<did>
data = {"id":N,"method":"miIO.ota","params":{"app_url":"http://<your-lan-ip>:8000/fw_crc.bin"}}
```

Two things about that payload matter:

- `app_url` alone is what worked here (`mcu_url` is the equivalent for the
  companion MCU). The larger payload shown in `python-miio` - `mode`, `install`,
  `file_md5`, `proc` - was not tested over the cloud channel, so this is what is
  known to work rather than the only form that does.
- No checksum is passed in this form, so nothing in the protocol protects you
  from shipping a bad image.

The cloud only relays the instruction. The firmware file never leaves your LAN.

## Two requirements that are easy to miss

### 1. The image carries a 4-byte CRC trailer

Xiaomi's own ESP32 update images are a normal ESP-IDF application image - whose
internal SHA-256 verifies - followed by four extra bytes. A plain ESPHome
`firmware.bin` has no trailer, so one was appended here to match the vendor
format. Whether the device rejects an image without it was not tested.

The algorithm is **not** a standard CRC-32; none of the catalogued variants
reproduce it:

```
CRC-32, polynomial 0x04C11DB7 (reflected 0xEDB88320)
        init 0x00000000, refin/refout = true, xorout 0x00000000
        stored little-endian
```

That is standard CRC-32 without the customary pre- and post-inversion. Verified
against two unrelated Xiaomi images - one ESP32, one MT7697 - which both
reproduce exactly.

`tools/append_crc.py` implements it, and can verify itself against any genuine
Xiaomi image you have.

### 2. The HTTP server must speak HTTP/1.1

The device's downloader identifies itself as `User-Agent: MIoT`. Against an
HTTP/1.0 server it connects, begins reading, then resets the connection - three
times in quick succession - and returns to `idle` with nothing written:

```
"GET /fw_crc.bin HTTP/1.1" 200 -
ConnectionResetError: [Errno 104] Connection reset by peer   (x3)
```

Python's `http.server` answers HTTP/1.0 and ignores `Range`, so it fails here.
Serving byte-identical content over HTTP/1.1 with keep-alive and range support
works first time. `tools/ota_server.py` is a minimal server that does this and
logs what the device actually requests.

## Procedure

1. **Recover the device token and `did`** from the Xiaomi cloud. Existing tools
   cover this; note the Yeelight app account and the Xiaomi account may be the
   same identity, in which case no re-pairing is needed.

2. **Build the ESPHome image** for your model. Include `ap:` and
   `captive_portal:` - see the warning below.

3. **Check it fits.** Only the application partition is written and the stock
   partition table stays, so the image must fit the slot the stock firmware uses.
   The stock partition table has not been dumped, so the exact slot size is
   unknown. The `lamp9` ESPHome build used here was 810 KB and installed without
   trouble.

4. **Append the CRC trailer:**

   ```
   python3 tools/append_crc.py firmware.bin fw_crc.bin
   ```

5. **Serve it over HTTP/1.1** and confirm another host on the LAN can fetch the
   whole file before going further:

   ```
   python3 tools/ota_server.py fw_crc.bin 8000
   ```

6. **Relay the OTA command through the cloud:**

   ```
   python3 tools/cloud_ota.py --server de --ip <device-ip> \
       --url http://<your-lan-ip>:8000/fw_crc.bin
   ```

7. **Watch it land.** `miIO.get_ota_state` walks `idle -> downloading ->
   installed`, the server logs one full-size `GET`, and the device reboots into
   ESPHome. The stock protocols (TCP 55443, UDP 54321) go silent.

## Always include a fallback AP

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "fallback-ap"

captive_portal:
```

A wrong credential is otherwise unrecoverable without UART - which defeats the
entire point of flashing over the network. This is not hypothetical: during this
work an SSID picked up a trailing `\r` from a CRLF file, the lamp came up on
ESPHome unable to join, and the fallback AP turned a teardown into a two-minute
captive-portal fix.

Worth knowing: credentials entered through the captive portal are stored in NVS
and **replace** the compiled-in ones (`set_sta`, not `add_sta`). They survive
subsequent OTA updates, so a device can keep working on portal-saved credentials
even while the firmware carries a wrong SSID.

## What is safe and what is not

Confirmed non-destructive on the reference unit:

- An OTA pointed at a URL returning **404** - the device fetches, fails, returns
  to `idle`, unharmed. A useful way to prove the path end to end without
  installing anything.
- Aborted transfers, including the HTTP/1.0 resets above.
- Unknown or malformed miIO methods, which are ignored without a reboot.

Not verified, and where the real risk lies:

- The **install** stage. Once a valid image downloads it is written and booted.
  A third-party report describes an official Yeelight OTA bricking a ceiling
  light, so this firmware's install path can write something unbootable.
- The OTA call itself carries no checksum, so nothing at the protocol level
  distinguishes a correct image from one built for the wrong model. What the
  firmware checks beyond the CRC trailer, and how it behaves on a bad image, was
  not tested - deliberately.
