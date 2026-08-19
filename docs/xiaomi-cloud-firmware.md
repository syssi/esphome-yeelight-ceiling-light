# Fetching stock firmware and metadata from the Xiaomi cloud

Converting a device to ESPHome is easier with the stock image in hand: it gives a
way back, and its size tells you what the application partition really holds.
Xiaomi will hand it over, from an endpoint the Mi Home app uses.

Everything below was tested against a live account. Where something is inferred
rather than observed it says so.

## Where the endpoint list came from

Xiaomi's own plugin SDK ships a registry of every cloud path the app may call:

- <https://github.com/MiEcosystem/miot-plugin-sdk> - `miot-sdk/service/apiRepo.js`
- the same repo's `miot-sdk/service/smarthome.js` documents a handful of them

That is a better starting point than guessing, and it is how the eight
firmware-related paths below were found rather than brute-forced.

## `/home/latest_version` - the one that returns a download

Request `{"model": "<model>"}`:

```json
{
  "version": "2.0.6_0049",
  "url":      "https://fk-res-abroad-cdn.home.mi.com/default/<md5>_upd_<model>.bin?GalaxyAccessKeyId=...&Expires=...&Signature=...",
  "safe_url": "<the same, signed>",
  "md5": "df39251190b94c50c518d0cbd10523f7",
  "changeLog": "", "fw_change_log": "", "mcu_change_log": "",
  "upload_time": 1591941617,
  "diff_url": "", "diff_safe_url": "", "diff_md5": "",
  "time_out": 30, "force": false, "rec": false
}
```

Two things about it are worth knowing.

Xiaomi's SDK documents this endpoint as returning version info **"for BLE
devices"**. It serves Wi-Fi miIO models perfectly well - verified against
`yeelink.light.ceiling10`, `lamp9` and `ceilb`, all ESP32 Wi-Fi devices.

It is keyed on the **model string only**. No `did`, and no check that the account
owns such a device: any account that can log in can fetch any model's current
firmware. That is what makes a catalogue of 120 models possible from one account
that owns three.

The URL signature is time limited - about two months on the responses seen here -
so re-request it rather than storing the link.

## Older versions cannot be downloaded

`/v2/device/get_firmware_history` `{"did": did}` lists previous versions with
their change descriptions. Those builds are not retrievable.

The endpoint does accept a `version` key, which makes this worth stating
carefully. Asking for the version that is already current succeeds and echoes it
back. Asking for any older version returns **the current build instead**, with no
error. Tested on three models with eight requests, three of them controls
requesting the current version:

| Requested | Returned |
| --------- | -------- |
| `ceiling10@2.0.6_0049` (control) | `2.0.6_0049` |
| `lamp9@2.1.7_0031` (control) | `2.1.7_0031` |
| `ceilb@2.1.7_0011` (control) | `2.1.7_0011` |
| `ceiling10@2.0.6_0042` | `2.0.6_0049` |
| `ceiling10@1.3.2_0028` | `2.0.6_0049` |
| `lamp9@2.1.7_0029` | `2.1.7_0031` |
| `lamp9@2.0.6_0017` | `2.1.7_0031` |
| `ceilb@2.0.6_0008` | `2.1.7_0011` |

The controls matter: all three passed on the first key tried, so the parameter
name is right and the server does read it. It simply refuses to serve anything
but the current build.

The failure is silent, which is the trap. Without comparing the returned version
against the requested one, all five would look like successes and write the
*latest* image into a file named for an older version - and md5 verification
would not catch it, because the md5 matches, it is just the md5 of the wrong
build. `cloud_fw_info.py` checks the echo and refuses the download.

So restore means restore-to-current, not restore-to-what-was-installed.

## The other paths, for completeness

| Path | Request | Returns |
| ---- | ------- | ------- |
| `/home/checkversion` | `{"did":did,"pid":0}` | `curr`, `latest`, `isLatest`, `ota_status`, `ota_progress`. No URL. |
| `/v2/device/check_device_version` | `{"did":did,"pid":0}` | identical to the above |
| `/v2/device/get_firmware_history` | `{"did":did}` | version list with descriptions. `{"model":...}` gives `invalid param` |
| `/v2/device/get_auto_upgrade_config` | `{"did":did}` | auto-update switches |
| `/v2/device/latest_ver` | `{"did":did}` | `-8 cant check did` for a Wi-Fi device; this one really is BLE-only |
| `/home/multi_checkversion` | `{"dids":[...]}` | the same version data for several devices |
| `/home/devupgrade` | - | **starts an upgrade.** Not probed. |

## Which models can run ESPHome

ESPHome targets ESP32, ESP8266, RP2040, LibreTiny (BK72xx, RTL87xx) and
experimentally nRF52. Yeelight's Zigbee and BLE-mesh lights use Telink parts,
which are not an ESPHome platform at all, so they are not candidates whatever
their firmware looks like.

Two filters, applied in that order, because the first is free:

**Protocol, from the public catalogue.** `home.miot-spec.com/p/<model>` carries a
`{"name":"Protocol","value":"Wi‑Fi"}` attribute. `tools/classify_models.py` reads
it. This is a community mirror, not Xiaomi's API, so it costs no rate-limited
calls. Note the value uses U+2011, a non-breaking hyphen, so a naive comparison
against `"Wi-Fi"` fails.

Of 253 `yeelink.light.*` models in Xiaomi's spec registry
(<https://miot-spec.org/miot-spec-v2/instances?status=all>), 120 come back Wi-Fi
or MiIO. The rest are 31 BLE Mesh, 1 Bluetooth, 4 virtual groups, 3 with an
unlabelled protocol id, and 94 with no product page at all.

As a check on the method, all 14 models this repo already ships working configs
for classify as Wi-Fi.

**Chip, from the image header.** Wi-Fi narrows it but does not prove ESP32 - the
older bulbs are ESP8266. Read the first 32 bytes with a `Range` request instead
of downloading:

- not `0xE9` at offset 0: not an ESP-IDF image, skip
- `chip_id` at offset 12..13 in the known set: that ESP32 variant
- otherwise, entry address in `0x40100000`..`0x402fffff`: ESP8266, whose header is
  only 8 bytes, so those two bytes are a segment length and read as a nonsense
  chip id

The `Content-Range` reply also gives the total size, so a model can be classified
and measured for 32 bytes.

Across the 120: **50 ESP32**, **9 ESP8266**, 17 with some other container, 44 with
no firmware offered. The 17 are consistently the lowest-numbered model in each
family - `ceiling1`-`6`, `color1`-`3`, `lamp1/3/5`, `mono1`, `strip1/2`, `ct2`,
`bslamp1` - all 300-750 KB, so first-generation hardware.

## Partition headroom

Stock ESP32 images in this family run **1.22-1.57 MB**. Any ESPHome build for one
of these devices is comfortably smaller, so the application partition has room.
This is measured across 50 models, not extrapolated from one.

## Logging in

Sessions are cached (`tools/mi_session.py`) because a Xiaomi session is only three
values - `userId`, `ssecurity`, `serviceToken` - and re-logging-in for every
invocation means a captcha and an emailed 2FA code every time. Xiaomi rate limits
those codes, with a daily ceiling that is reachable in an afternoon.

`--qr` logs in by QR code scanned in the Mi Home app, which skips the captcha.

One observation that is a hypothesis rather than a finding: the token extractor
generates a random `deviceId` per instance and sets it as a cookie during login,
so every login introduces itself as new hardware - plausibly why each one draws a
2FA challenge. `mi_session.py` pins a stable `deviceId` and User-Agent across
runs. Whether that reduces the challenges has not been confirmed.

State lives in `~/.cache/esphome-yeelight` by default, `MI_STATE_DIR` to move it.
The session file holds live credentials; it is written `0600`.
