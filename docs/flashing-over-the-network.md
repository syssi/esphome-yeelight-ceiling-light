# Flashing over the network, without opening the device

Every documented conversion in this project flashes over UART, which means taking
the device apart. On ESP32 Yeelights running stock firmware there is another
route: the device's own `miIO.ota` update mechanism can be pointed at a file on
your LAN.

Confirmed here on two units of different firmware generations:
`yeelink.light.lamp9` on `2.1.7_0031` (`miio_ver 0.0.9`) and
`yeelink.light.ceiling10` on `2.0.6_0049` (`miio_ver 0.0.6`). The requirements
differ between them - see the table further down. Third parties report the same
mechanism working on `yeelink.light.ceiling22`; that was not verified here.

> **Read the limits first.** The procedure here writes only the application
> partition. The stock bootloader and partition table remain, so it cannot recover
> a device that will not boot - that still needs UART, since a device that does not
> boot is not on the network either. (ESPHome does document writing the partition
> table and bootloader over the air; that was not tested here - see "Replacing the
> partition table or bootloader" below.)
>
> It is worth knowing what recovery you do have. The vendor's current stock image
> for a given model can be fetched from Xiaomi's cloud - see
> `docs/xiaomi-cloud-firmware.md` - so a way back exists without a UART backup,
> though only to the *current* build, since older versions are not downloadable.
> Separately, the updater writes to the inactive OTA slot, so the stock app
> survives in the other one - but only until the *next* flash, which targets it.
> See "What the flash looks like underneath" below.
>
> That surviving copy can be read out **over the network**, and written back the
> same way, with no UART and no teardown - which also covers the case the cloud
> cannot: a build older than current, or a model the endpoint does not serve. It
> has to be set up before the conversion, though, because the image that replaces
> stock is the only one that can still reach it. See "Reading the stock image out
> over the network" below, and enable `flash_probe.yaml` at step 2.

---

## The route that works

Xiaomi's cloud exposes an RPC relay that forwards a raw miIO call to a device you
own. Because the command then originates from the server, the OTA service accepts
it - and `app_url` may point anywhere the device can reach, including a private
LAN address over plain HTTP.

```
POST https://<region>.api.io.mi.com/app/home/rpc/<did>
data = {"id":N,"method":"miIO.ota","params":{"app_url":"http://<your-lan-ip>/fw_crc.bin"}}
```

Two things about that payload matter:

- `app_url` alone is what worked here (`mcu_url` is the equivalent for the
  companion MCU). The larger payload shown in `python-miio` - `mode`, `install`,
  `file_md5`, `proc` - was not tested over the cloud channel, so this is what is
  known to work rather than the only form that does.
- No checksum is passed in this form, so nothing in the protocol protects you
  from shipping a bad image.

The cloud only relays the instruction. The firmware file never leaves your LAN.

## What is safe and what is not

Confirmed non-destructive on the reference unit:

- An OTA pointed at a URL returning **404** - the device fetches, fails, returns
  to `idle`, unharmed. A useful way to prove the path end to end without
  installing anything.
- Aborted transfers, including the HTTP/1.0 resets described in requirement 2.
- Unknown or malformed miIO methods, which are ignored without a reboot.

Not verified, and where the real risk lies:

- The **install** stage. Once a valid image downloads it is written and booted.
  A third-party report describes an official Yeelight OTA bricking a ceiling
  light, so this firmware's install path can write something unbootable.
- The OTA call itself carries no checksum, so nothing at the protocol level
  distinguishes a correct image from one built for the wrong model. What the
  firmware checks beyond the CRC trailer, and how it behaves on a bad image, was
  not tested - deliberately.

## Five requirements that are easy to miss

Not all five apply to every device, and the differences track the firmware
generation. Two units were converted, and this is what each one actually showed:

| | `yeelink.light.lamp9`<br>`miio_ver 0.0.9`, stock 2.1.7_0031 | `yeelink.light.ceiling10`<br>`miio_ver 0.0.6`, stock 2.0.6_0049 |
| --- | --- | --- |
| 1. CRC trailer | appended; necessity not tested | **required** - verified by serving an image without one |
| 2. HTTP/1.1 | **observed necessary** | served over 1.1 throughout, so never retested |
| 3. no port in the URL | **not needed** - flashed successfully on port 8000 | **required** |
| 4. `PARTITION_TABLE_MD5: n` | **not needed** - booted fine without it | **required** |
| 5. `FREERTOS_UNICORE: y` | set; necessity not tested | **required**; die confirmed single-core |

So a newer unit may well flash without 3 or 4 at all. An older one needs both, and
each failure looks like something else entirely - which is why they are written up
in detail below rather than as a checklist.


### 1. The image carries a 4-byte CRC trailer

Xiaomi's own ESP32 update images are a normal ESP-IDF application image - whose
internal SHA-256 verifies - followed by four extra bytes. A plain ESPHome
`firmware.bin` has no trailer, so one must be appended.

**This is enforced.** Serving a `ceiling10` an image with no trailer - byte-identical
to one it had already accepted and booted, minus the four bytes - it downloads the
whole file, rejects it, and retries twice more before giving up:

```
[I] miio_ota: 1 ota task comming...
[I] httpc: Content-Length 804704
[I] httpc: ==> 0% ... ==> 100%
[I] httpc: GET Done(804704bytes).
[I] httpc: File Done(804704bytes).
[E] arch_ota: crc check failed!  (arch_ota_check_crc,309)
[E] ota_app: ota erro count:1 (app_fw_error,366)
[W] miio_ota: error count = 1 (ota_virtual_error,398)
        ... two more identical attempts ...
[E] miio_ota: error occurred too many times (ota_virtual_error,406)
[I] miio_ota: installed = 0, failed = 1
```

Three complete downloads and no install. Note the shape of that failure, because
it is visible without a serial console: **three full-size GETs in the HTTP server
log, then `miIO.get_ota_state` returning to `idle`** with the device still on stock.
A trailered image produces exactly one GET and then `installed`.

The device is left untouched by a rejection, so this is a safe thing to get wrong.

The vendor's own function name, `arch_ota_check_crc`, also confirms the trailer is
a CRC over the image rather than any other kind of tag.

The algorithm is **not** a standard CRC-32; none of the catalogued variants
reproduce it:

```
CRC-32, polynomial 0x04C11DB7 (reflected 0xEDB88320)
        init 0x00000000, refin/refout = true, xorout 0x00000000
        stored little-endian
```

That is standard CRC-32 without the customary pre- and post-inversion. Verified
against five unrelated Xiaomi images which all reproduce exactly: one MT7697, and
the stock ESP32 images for `yeelink.light.ceiling10`, `lamp9`, `ceilb` and one
further ESP32 product.

`tools/append_crc.py` implements it, and can verify itself against any genuine
Xiaomi image you have.

**The trailer is stored in flash, not consumed by the updater.** Reading a
converted `lamp9`'s stock slot back shows the image ending at its ESP-IDF length
of 1,494,080 bytes with `0414c948` in the four bytes immediately after - the same
trailer the served file carried. Two things follow. Restoring such an image
byte-for-byte reproduces what the vendor wrote, trailer included; and
`esp_image_verify()` accepts it, because bytes past the declared image end are
never examined. Both were confirmed by restoring one.

### 2. The HTTP server must speak HTTP/1.1

Observed on `lamp9` (`miio_ver 0.0.9`). The device's downloader identifies itself
as `User-Agent: MIoT`. Against an HTTP/1.0 server it connects, begins reading,
then resets the connection - three times, the updater's own retry limit, the same
one seen in requirement 1 - and returns to `idle` with nothing written:

```
"GET /fw_crc.bin HTTP/1.1" 200 -
ConnectionResetError: [Errno 104] Connection reset by peer   (x3)
```

Python's `http.server` answers HTTP/1.0 and ignores `Range`, so it fails here.
Serving byte-identical content over HTTP/1.1 with keep-alive and range support
works first time. `tools/ota_server.py` is a minimal server that does this and
logs what the device actually requests.

Every later transfer, `ceiling10` included, was served over HTTP/1.1 by that tool,
so the failure was never reproduced on the older generation - it simply never had
the chance to occur.

### 3. The URL must not contain a port

The updater does not strip the port from the URL authority. Given
`http://192.0.2.2:8000/fw.bin` it issues a DNS lookup for the literal string
`192.0.2.2:8000`:

```
192.0.2.10.24408 > 192.0.2.1.53: A? 192.0.2.2:8000.
192.0.2.1.53 > 192.0.2.10.24408: NXDomain
```

Three attempts, three NXDOMAINs, then it gives up. `miIO.ota` still answers
`["ok"]`, `miIO.get_ota_state` still reads `idle`, and nothing arrives at the HTTP
server - so the symptom is indistinguishable from the command being ignored.

**Serve on port 80** so the authority is a bare address. It is then recognised as
an IP literal, never goes to DNS, and the transfer starts immediately.

This cost a long detour to find, because it is invisible from the machine running
the HTTP server: a switch does not forward the device's DNS queries to another
port, so a capture there shows nothing either way. It was only visible from the
router.

**This is generation-specific.** Observed on `ceiling10` (`miio_ver 0.0.6`). The
`lamp9` on `0.0.9` does **not** have the bug: it was flashed successfully with
`--url http://<ip>:8000/fw_crc.bin`, which is why the procedure below originally
specified port 8000.

Port 80 is nevertheless the right default, because it works on both and costs
only a `sudo`.

#### Corollary: what counts as success

`["ok"]` is the cloud acknowledging the relay, not the device agreeing to do
anything. On `ceiling10` (`0.0.6`), `miIO.ota` also reboots the device about eight
seconds after it is accepted whether or not a download follows, so a failed
attempt power-cycles the light. **Watch the HTTP server log for a GET from the
device**; that is the only evidence the transfer started, and
`miIO.get_ota_state` moving `idle -> downloading -> installed` confirms it.

`tools/cloud_ota.py --sweep` tries several documented payload shapes in one login
and stops as soon as the device leaves `idle`, which is useful when a firmware
generation wants a different payload - though note that no payload shape helps if
the URL carries a port.

### 4. The app must tolerate a partition table with no MD5

**Also generation-specific**, and observed on `ceiling10` (`miio_ver 0.0.6`); the
`lamp9` on `0.0.9` booted an image built without this option, so its stock table
does carry the MD5 record.

This one does not stop the transfer. The image downloads, the updater reports
`installed`, and then the device is bricked in a reboot loop - **silent at every
layer**. No Wi-Fi, no fallback AP, and power-cycling cannot reach ESPHome's safe
mode either. Only UART reveals why:

```
E (344) partition: No MD5 found in partition table
E (345) partition: load_partitions returned 0x105
assert failed: esp_ota_get_running_partition esp_ota_ops.c:721 (it != NULL)
```

ESP-IDF's `CONFIG_PARTITION_TABLE_MD5` defaults to `y`, and
`components/esp_partition/partition.c` refuses a table with no MD5 record when it
is set. Its own Kconfig help says the generation "should be turned off for legacy
bootloaders which cannot recognize the MD5 checksum in the partition table" -
which is exactly the situation here, because the stock partition table on the
older firmware generation carries no such record.

`esp_ota_get_running_partition()` then finds nothing and the app asserts **before
Wi-Fi and before `safe_mode` set up**, which is why none of the usual recovery
paths exist. Recovery is UART.

The fix is one option:

```yaml
esp32:
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_PARTITION_TABLE_MD5: n
```

Nothing on flash changes - the stock bootloader, partition table and layout are
all left alone; the app simply stops requiring a record that was never there.

Confirm it applied before flashing, because a disabled bool is written as a
comment rather than `=n`:

```
grep PARTITION_TABLE_MD5 .esphome/build/<name>/sdkconfig.<name>
# CONFIG_PARTITION_TABLE_MD5 is not set

strings .pioenvs/<name>/firmware.bin | grep -c "No MD5 found in partition table"
0
```

Setting the option is harmless on a device that does not need it, so it is worth
having on any config intended for this route.

### 5. Build for a single core if the die has one

The `ceiling10`'s module is marked `ESP32-WROOM-32D`, which is normally the
dual-core `D0WD`. The die in it is not:

```
Chip type: Unknown ESP32 (revision v1.0)
Features:  Wi-Fi, BT, Single Core + LP Core, 240MHz
```

An image built for two cores tries to bring up an APP CPU that is not there, and
faults during startup - before Wi-Fi, so it looks exactly like requirement 4.

```yaml
esp32:
  framework:
    type: esp-idf
    sdkconfig_options:
      CONFIG_FREERTOS_UNICORE: y
```

Every config in this repository already sets this, which makes it easy to overlook
when writing a new one from scratch. It was set for the `lamp9` too, so whether
that unit strictly needs it is unknown - its die was never read.

The lesson is narrower than "check the die": the module marking does not tell you,
so set the option rather than trusting the label.

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

Worth knowing how portal-saved credentials behave, because it is easy to get
wrong. They are stored in NVS and **replace** the compiled-in ones (`set_sta`, not
`add_sta`) - so a device can keep running on them while the firmware carries a
wrong SSID. But the preference is keyed on `App.get_config_version_hash()`, so
**any configuration change orphans them** and the device falls back to whatever is
compiled in. They persist across a plain re-flash of the same config, not across
an edited one.

## Procedure

1. **Set up cloud access.** There is nothing to look up by hand: `tools/cloud_ota.py`
   finds the device's `did` itself, by matching the `--ip` you give it against the
   device list on your account. The device's LAN token is not needed either - the
   command is relayed by the cloud, not sent locally, so the token only matters for
   the local call that does not work anyway (see the appendix).

   Logging in is delegated to
   [Xiaomi-cloud-tokens-extractor](https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor),
   which handles the captcha and email 2FA that Xiaomi's login now requires. It is
   not vendored here, so clone it where the tools expect it, or point `--extractor`
   at a checkout elsewhere:

   ```
   git clone https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor \
       tools/Xiaomi-cloud-tokens-extractor
   ```

   Credentials are read from the environment rather than passed as arguments.
   Prompt for the password instead of typing it into the command line, so it stays
   out of your shell history:

   ```
   export MI_USERNAME=you@example.com
   read -rsp 'Mi password: ' MI_PASSWORD && export MI_PASSWORD
   ```

   `--qr` logs in by scanning a code in the Mi Home app instead, which needs no
   password at all and skips both the captcha and the emailed code - worth reaching
   for, since Xiaomi rate limits those codes. Sessions are cached by
   `tools/mi_session.py` and reused until the server rejects them, so this is
   usually a one-time cost. See "Logging in" in
   [xiaomi-cloud-firmware.md](xiaomi-cloud-firmware.md) for the details, including
   where the cache lives.

   `--server` selects the account region (`cn`, `de`, `us`, `ru`, `tw`, `sg`, `i2`;
   default `de`). The device list is per-region, so the wrong one simply will not
   find your lamp.

   Note the Yeelight app account and the Xiaomi account may be the same identity,
   in which case no re-pairing is needed.

   Confirm the whole path before flashing anything. This only reads:

   ```
   python3 tools/cloud_ota.py --server de --ip <device-ip> --state
   ```

   It prints the matched device - name, model and `did` - and its OTA state, which
   is `idle` on a lamp that has not been touched.

2. **Build the ESPHome image** for your model. Include `ap:` and
   `captive_portal:` - see "Always include a fallback AP" above, which explains
   why this is not optional.

   If you want a copy of the vendor firmware, uncomment the `flash_probe.yaml`
   include at the top of the device config **now**. This is the only flash after
   which stock is still readable: the next one overwrites it, so an image without
   this cannot be used to rescue the copy it is sitting next to.

3. **Check it fits.** Only the application partition is written and the stock
   partition table stays, so the image must fit the slot the stock firmware uses.
   The slots are **1,966,080 bytes** (0x1E0000) on both models dumped here, and
   stock itself is about 1.4 MB, so a typical ESPHome build has well over a
   megabyte of headroom. The `lamp9` builds used here were 810 KB and 828 KB.

4. **Append the CRC trailer:**

   ```
   python3 tools/append_crc.py firmware.bin fw_crc.bin
   ```

5. **Serve it over HTTP/1.1** and confirm another host on the LAN can fetch the
   whole file before going further:

   Port **80**, not a high port. Older firmware cannot parse a URL with an
   explicit port (requirement 3), and port 80 works on every unit tested, so it is
   the safe default - at the cost of needing root to bind:

   ```
   sudo python3 tools/ota_server.py fw_crc.bin 80
   ```

6. **Relay the OTA command through the cloud**, with no port in the URL:

   ```
   python3 tools/cloud_ota.py --server de --ip <device-ip> \
       --url http://<your-lan-ip>/fw_crc.bin
   ```

7. **Watch it land.** `miIO.get_ota_state` walks `idle -> downloading ->
   installed`, the server logs one full-size `GET`, and the device reboots into
   ESPHome. The stock protocols (TCP 55443, UDP 54321) go silent.

## What the flash looks like underneath

Worth knowing before starting, because it determines what recovery is available.
Read the table with `esptool read-flash 0x8000 0xc00 ptable.bin` over UART, or
without opening the device by enabling `flash_probe.yaml`, whose `debug`
component logs the whole table at boot.

Dumped on two units of different firmware generations - `ceiling10` on `2.0.6`
over UART, `lamp9` on `2.1.7` over the network - and the two are **identical**,
label for label and byte for byte. That is worth knowing before assuming a new
model differs, though it is still two data points.

| label | type | subtype | offset | size |
| ----- | ---- | ------- | ------ | ---- |
| nvs | data | nvs | 0x9000 | 16K |
| otadata | data | otadata | 0xD000 | 8K |
| phy_init | data | phy | 0xF000 | 4K |
| miio_fw1 | app | ota_0 | 0x10000 | 1920K |
| miio_fw2 | app | ota_1 | 0x1F0000 | 1920K |
| test | app | test | 0x3D0000 | 76K |
| mfi_p | data | spiffs | 0x3E3000 | 4K |
| factory_nvs | data | nvs | 0x3E4000 | 16K |
| coredump | data | coredump | 0x3E8000 | 64K |
| minvs | data | 0xfe | 0x3F8000 | 16K |

Four things follow from it.

**The app slots are 1920 KB**, so image size is a non-issue: stock itself is about
1.3 MB and a typical ESPHome build for one of these is well under a megabyte.

**There is no `factory` partition**, so `otadata` alone decides what boots.

**Stock survives the first flash, and only the first.** Both the vendor updater
and ESPHome's own OTA write to whatever `esp_ota_get_next_update_partition()`
returns, which with two slots and no `factory` partition strictly alternates. So
the conversion lands in the free slot and leaves the vendor app intact; the *next*
flash targets the slot holding it. If you want that copy, read it out before the
second flash - over the network as described below, or over UART per
`docs/flashing-over-uart.md`.

**Configuration is not in the app slots at all**, which is what makes going back
and forth survivable. Details below.

### Configuration lives outside the app slots

There is exactly one `nvs` partition, at `0x9000`, and both firmwares share it.
No app slot holds any configuration, which is why changing which slot boots does
not lose your Wi-Fi credentials or the vendor pairing - the stock firmware comes
back already knowing them.

That is by design rather than luck. `esp_ota_begin()` erases the whole destination
app partition before writing, so anything kept inside an app partition would die
on every update. ESP-IDF therefore separates code from settings, and NVS isolates
writers by **namespace** rather than by partition: ESPHome opens
`nvs_open("esphome", ...)` and the stock firmware uses its own names. Keys cannot
collide, but the 16 KB and its free space are shared, so the two are isolated
logically, not physically.

The captive-portal behaviour described earlier is the same mechanism one level
down - those credentials are NVS entries keyed on
`App.get_config_version_hash()`, which is why a configuration change orphans them.

Two paths erase the *whole* `nvs` partition, vendor keys included:

- the `factory_reset` component - button, switch or action - calls
  `global_preferences->reset()`, which calls `nvs_flash_erase()`;
- failing to open the `esphome` namespace at startup, after which ESPHome erases
  NVS and retries. A full NVS would trigger this silently.

So keep `factory_reset` out of any configuration for a device whose vendor pairing
you want to survive a return to stock. `nvs_flash_erase()` acts only on the
partition named `nvs`, so `factory_nvs` and `minvs` are not affected by it.

What the other data partitions hold has **not been established** - none of them
have been read. Going by labels, subtypes and sizes alone: `factory_nvs` is a
second NVS whose name suggests factory-provisioned identity, `minvs` uses a custom
subtype (`0xfe`) and so is presumably miio-specific, `mfi_p` is a 4 KB SPIFFS
whose name points at Apple MFi and therefore plausibly HomeKit pairing, and
`phy_init` holds RF calibration. Which partition stores the device token is
unknown for the same reason.

### Getting stock back without reflashing it

`otadata` holds one 32-byte record per 4K sector, at `0xD000` and `0xE000`. Each
carries a sequence number; the highest valid one wins and selects
`ota_[(seq-1) % 2]`. After a conversion you will typically find the older record
still pointing at the slot stock lives in.

So erase the newer record and let the older one win:

```
esptool erase-region 0xE000 0x1000
```

NVS survives, so Wi-Fi credentials and the vendor pairing stay intact and the
device rejoins its account by itself.

Do **not** erase the whole of `otadata`: with no valid record the bootloader falls
back to the *first* app partition, which is `ota_0` - not necessarily stock.

The records were read off a converted `lamp9` and behave exactly as described:
`seq` 4 and 3, the higher one flagged `ota_state = 0x02` (valid) and the other
`0xFFFFFFFF` (undefined), selecting `ota_[(4-1) % 2]` = `ota_1` = `miio_fw2` -
which was the slot the device reported running.

### Reading the stock image out over the network

The `otadata` trick above only works while the vendor copy is still in a slot. It
does not survive a second flash, and it cannot help at all on a device whose stock
build the cloud no longer serves. Reading the image out gives you a file instead,
and needs no UART.

**Set it up before converting.** Uncomment the include at the top of the device
config:

```yaml
packages:
  flash_probe: !include flash_probe.yaml
```

That adds four read-only API actions - `dump_flash`, `dump_otadata`, `log_slots`,
`redump_config` - implemented in `tools/probe_flash.h`. It does **not** enable
`allow_partition_access`, which gates partition-table and bootloader *writes* and
carries its own bricking warning; none of this needs it.

`dump_flash` reads `esp_ota_get_next_update_partition()` - the slot this build is
not running from, which after exactly one flash is the one holding stock - and
logs it as base64, one chunk per line. `tools/dump_stock.py` drives that loop,
reassembles the chunks and is restartable, because it records completed offsets
in a sidecar next to the output file:

```
python3 tools/dump_stock.py <device-ip> stock.bin
```

The whole slot comes out, 960 chunks of 2048 bytes on these models. On the run
documented here that took about six minutes with no retries.

**Verify it against something.** The point of the exercise is a file you can
trust, so check it against an independent oracle rather than eyeballing it. If the
cloud still serves your model's build, `docs/stock-firmware-catalogue.md` has the
md5 - and a match proves the technique rather than merely producing plausible
bytes. The `lamp9` `2.1.7_0031` extraction reproduced the catalogue md5
`08f09790930817d39d02cb2d5dac1840` exactly, trailer included.

**Do not truncate trailing `0xFF` to find the end of the image.** A slot is not
image-then-erase-pattern. The updater erases only the sectors it writes, so
whatever the *previous* firmware left beyond the current image is still there. The
`lamp9` slot read back as:

| region | contents |
| ------ | -------- |
| 0x000000 - 0x16CC43 | the image, plus its 4-byte CRC trailer |
| 0x16CC44 - 0x16CFFF | `0xFF`, padding to the sector boundary |
| 0x16D000 - 0x18FD8F | 142,736 bytes left over from an earlier, larger build |
| 0x18FD90 - end | `0xFF` |

Trimming trailing `0xFF` would have yielded 1,637,776 bytes - not the image, and
not anything that would boot. Parse the ESP-IDF header instead:

```
python3 tools/parse_esp32_img.py stock.bin
```

which reports the segment structure, checks the appended SHA-256, and prints where
the legitimate image ends and how many bytes follow it.

Those leftover sectors are also worth a thought before sharing a raw slot dump:
they contain fragments of whatever occupied the partition previously.

### Putting a stock image back over the network

ESPHome's own OTA accepts a foreign image, so a stock build can be written back
the same way it came out - no UART, and no need for the device to still be paired
to the cloud:

```
esphome upload --file stock.bin <device-config>.yaml --device <device-ip>
```

Underneath, this is an ordinary `OTA_TYPE_UPDATE_APP` write - unrestricted, and
nothing to do with `allow_partition_access`. It targets
`esp_ota_get_next_update_partition()`, so it lands in the slot the running build
is not using; then `esp_ota_end()` runs `esp_image_verify()` and, if that passes,
`esp_ota_set_boot_partition()` flips `otadata`. Both images survive - you end up
with stock in one slot and ESPHome in the other, booting stock.

Keep the CRC trailer on. It is what the vendor writes, and `esp_image_verify()`
ignores bytes past the declared image end.

Confirmed end to end on a `lamp9`: 1,494,084 bytes uploaded in 9.6 s, the device
rebooted onto stock `2.1.7_0031`, TCP 55443 answered again, and - because NVS is
untouched - the vendor app drove the lamp immediately without re-pairing. The
image used was the one extracted from that same lamp, so what this demonstrates is
the full round trip rather than a checksum.

**How to test a restore honestly.** Writing stock into a slot that already holds
byte-identical stock proves nothing: a write that did nothing at all would leave a
valid stock image in place, `otadata` would still flip, and the device would still
boot stock. Success and no-op are indistinguishable. Overwrite the target slot
with something else first - flashing the ESPHome build a second time will do it,
since that targets the inactive slot - and confirm the slot really changed by
reading its first chunk back with `dump_flash`. Only then push the stock image.
Booting stock afterwards has exactly one explanation.

**The risk, stated plainly.** The good failure is `esp_image_verify()` rejecting
the image: the OTA aborts and nothing changes. The bad failure is a written but
unbootable image. These devices log `Bootloader too old for OTA rollback` at boot,
so there is no automatic recovery from that - it means UART. Bounded, but real.

### Replacing the partition table or bootloader

**Not tested in this project.** What follows is what ESPHome documents as
supported, recorded so the limits above are not mistaken for a hard boundary.
Nothing in the procedures on this page exercises it, and upstream's own warning is
that getting it wrong bricks the device past network recovery - which on these
lamps means UART, and opening them.

Everything else on this page writes only the application partition, using
`OTA_TYPE_UPDATE_APP` (0x00), which needs no special permission. ESPHome also
defines `OTA_TYPE_UPDATE_PARTITION_TABLE` (0x01) and `OTA_TYPE_UPDATE_BOOTLOADER`
(0x02), gated behind an opt-in:

```yaml
ota:
  - platform: esphome
    allow_partition_access: true    # ESP32 only, default false
```

With that in the **running** build:

```
esphome upload --partition-table <device-config>.yaml --device <device-ip>
esphome upload --bootloader      <device-config>.yaml --device <device-ip>
```

Four constraints worth knowing before planning around it:

- The flag must already be on the device. Enabling it costs one ordinary app
  flash first - and on a converted lamp that flash is the one that overwrites the
  surviving stock slot, so sequence it deliberately.
- The CLI refuses the options without the flag, and the device rejects the
  handshake independently with "Device only supports app updates".
- `--partition-table` and `--bootloader` cannot be combined, and neither works
  over serial - they are OTA-only paths.
- A newer bootloader is what would clear the `Bootloader too old for OTA rollback`
  warning these devices log, and restore rollback protection. That is the obvious
  motivation; it is also the change with the least margin for error, since a bad
  bootloader leaves nothing to recover with.

## Appendix: why the obvious approach fails

Background, kept for the record. Nothing below is needed to follow
the procedure above.

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
