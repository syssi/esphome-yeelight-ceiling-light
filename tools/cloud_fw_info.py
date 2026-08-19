#!/usr/bin/env python3
"""Fetch stock firmware metadata and images for Xiaomi devices from the cloud.

The winning endpoint is `/home/latest_version`, taking `{"model": <model>}`:

    {"version": "2.0.6_0049",
     "url":      "https://fk-res-abroad-cdn.home.mi.com/default/<md5>_upd_<model>.bin?...",
     "safe_url": "<same, signed>",
     "md5":      "df39251190b94c50c518d0cbd10523f7",
     "upload_time": 1591941617, "changeLog": "", "diff_url": "", ...}

Two things about it are worth knowing. Xiaomi's own SDK documents it as returning
version info "for BLE devices", but it serves Wi-Fi miIO models perfectly well -
verified against yeelink.light.ceiling10. And it is keyed on the **model string
only**, so it needs no `did` and no ownership of the device: an account that can
log in can fetch any model's current firmware. The signature on the URL is
time-limited (roughly two months), so re-run rather than saving the link.

`/home/checkversion` {"did":did,"pid":0} gives the version a specific device is
running and whether it is current, but never a URL. `/v2/device/get_firmware_history`
{"did":did} lists previous versions with changelogs - versions only, no URLs, so
older firmware still cannot be downloaded.

The path list came from Xiaomi's official plugin SDK, which ships an API registry:

    https://github.com/MiEcosystem/miot-plugin-sdk
      miot-sdk/service/apiRepo.js     - every path the app may call
      miot-sdk/service/smarthome.js   - what a few of them take and return

/home/devupgrade is deliberately absent from the probe list: it starts an upgrade.

Credentials come from the environment so they stay out of shell history:

    export MI_USERNAME=...
    export MI_PASSWORD=...

Usage:
    cloud_fw_info.py --model yeelink.light.ceiling10 --download firmware/
    cloud_fw_info.py --ip 192.0.2.10                 # probe one device fully
"""

import argparse
import hashlib
import json
import os
import pathlib
import sys
import time
import urllib.request


def _ensure_venv() -> None:
    """Re-exec into the local venv if started with a bare python3.

    Compare sys.prefix, not the interpreter path: venv/bin/python3 is a symlink to
    the system python, so resolving both compares equal and this never fires.
    """
    from pathlib import Path

    venv_dir = Path(__file__).resolve().parent.parent / "venv"
    venv_py = venv_dir / "bin" / "python3"
    if venv_py.is_file() and Path(sys.prefix) != venv_dir:
        os.execv(str(venv_py), [str(venv_py), *sys.argv])


# Unconditional: a bare python3 may well have some of the dependencies (Crypto
# lives in ~/.local here) but not all of them, so "can I import one of them" is
# not a usable test for being in the right interpreter.
_ensure_venv()

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import mi_session  # noqa: E402

# ESP-IDF esp_image_header_t: magic at 0, segment count at 1, entry at 4..7,
# chip_id at 12..13. Anything not starting 0xE9 is not an ESP-IDF app image at
# all, which is how the non-ESP models (BLE/mesh parts) get filtered out.
ESP_CHIP_IDS = {
    0x0000: "ESP32", 0x0002: "ESP32-S2", 0x0005: "ESP32-C3",
    0x0009: "ESP32-S3", 0x000C: "ESP32-C2", 0x000D: "ESP32-C6",
    0x0010: "ESP32-H2", 0x0012: "ESP32-P4",
}


def identify_chip(url: str):
    """(chip, total_size) from a 32-byte ranged read. chip is None if not ESP-IDF.

    Costs one small CDN request instead of a whole firmware download, so a wide
    sweep can be filtered before committing to any bandwidth.
    """
    req = urllib.request.Request(url, headers={"Range": "bytes=0-31"})
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            head = r.read(32)
            crange = r.headers.get("Content-Range") or ""
            total = int(crange.rsplit("/", 1)[-1]) if "/" in crange else None
    except Exception as e:
        return f"probe failed: {e}", None

    if len(head) < 24 or head[0] != 0xE9:
        return None, total

    # ESP32 images carry a 24-byte extended header with chip_id at 12..13. ESP8266
    # images share the 0xE9 magic but have only an 8-byte header, so those two
    # bytes are the first segment's length there and read as a nonsense chip_id.
    # Tell them apart by where the code loads: ESP32 IRAM is 0x40080000, while
    # ESP8266 uses 0x40100000 (IRAM) and 0x40200000+ (flash-mapped irom0).
    chip_id = int.from_bytes(head[12:14], "little")
    if chip_id in ESP_CHIP_IDS:
        return ESP_CHIP_IDS[chip_id], total
    entry = int.from_bytes(head[4:8], "little")
    if 0x40100000 <= entry < 0x40300000:
        return "ESP8266", total
    return f"unknown chip_id 0x{chip_id:04x}", total


def list_devices(connector, server: str):
    """Every device on the account, own and shared."""
    homes = []
    h = connector.get_homes(server)
    if h:
        homes.extend({"home_id": x["id"], "home_owner": connector.userId}
                     for x in h["result"]["homelist"])
    cnt = connector.get_dev_cnt(server)
    if cnt:
        homes.extend({"home_id": x["home_id"], "home_owner": x["home_owner"]}
                     for x in cnt["result"]["share"]["share_family"])

    out = []
    for home in homes:
        devs = connector.get_devices(server, home["home_id"], home["home_owner"])
        if not devs or not devs.get("result", {}).get("device_info"):
            continue
        out.extend(devs["result"]["device_info"])
    return out


def find_device(connector, server: str, ip: str):
    for d in list_devices(connector, server):
        if d.get("localip") == ip:
            return d
    return None


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--ip", help="device LAN address; probes every per-device endpoint for it")
    p.add_argument("--model", action="append", default=[], metavar="MODEL",
                   help="fetch latest firmware info for this model (repeatable)")
    p.add_argument("--raw", action="store_true",
                   help="dump the full JSON for every model, not a summary line")
    p.add_argument("--models-from", metavar="FILE",
                   help="read model names from FILE, one per line (# comments ok)")
    p.add_argument("--esp-only", action="store_true",
                   help="identify the chip from a 32-byte ranged read first and "
                        "skip anything that is not an ESP-IDF app image")
    p.add_argument("--throttle", type=float, default=0.5, metavar="SEC",
                   help="pause between cloud calls (default: 0.5)")
    p.add_argument("--all-models", action="store_true",
                   help="fetch every distinct model on the account")
    p.add_argument("--version", metavar="VER",
                   help="ask for this specific firmware version rather than the "
                        "latest; undocumented, so it may simply be ignored. A "
                        "model may also carry its own as 'model@version'.")
    p.add_argument("--download", metavar="DIR",
                   help="save each model's image to DIR and verify its md5")
    p.add_argument("--qr", action="store_true",
                   help="log in by QR code scanned in the Mi Home app: no captcha "
                        "and no emailed 2FA code")
    p.add_argument("--server", default="de",
                   help="Xiaomi cloud region: cn, de, us, ru, tw, sg, i2 (default: de)")
    p.add_argument("--extractor",
                   default=str(pathlib.Path(__file__).resolve().parent
                               / "Xiaomi-cloud-tokens-extractor"),
                   help="path to a Xiaomi-cloud-tokens-extractor checkout")
    args = p.parse_args()

    if args.models_from:
        for line in pathlib.Path(args.models_from).read_text().splitlines():
            line = line.split("#", 1)[0].strip()
            if line:
                args.model.append(line)

    if not args.ip and not args.model and not args.all_models:
        p.error("give --ip, --model, --models-from or --all-models")

    # QR login never touches the password, so only demand credentials without it.
    if not args.qr:
        for var in ("MI_USERNAME", "MI_PASSWORD"):
            if not os.environ.get(var):
                print(f"{var} is not set", file=sys.stderr)
                return 2

    sys.argv = ["token_extractor.py",
                "-u", os.environ.get("MI_USERNAME", "").strip(),
                "-p", os.environ.get("MI_PASSWORD", "").strip(),
                "-l", "CRITICAL"]
    te = mi_session.load_extractor(args.extractor, qr=args.qr)

    connector = mi_session.get_connector(te, args.server, qr=args.qr)
    if connector is None:
        print("login failed", file=sys.stderr)
        return 1

    base = connector.get_api_url(args.server)

    def probe(path, payload, quiet=False):
        url = base + path
        if not quiet:
            print(f"\n>>> POST {path}  {json.dumps(payload)}", flush=True)
        try:
            resp = connector.execute_api_call_encrypted(url, {"data": json.dumps(payload)})
        except Exception as e:
            print(f"<<< raised {type(e).__name__}: {e}", flush=True)
            return None
        if not quiet:
            print(f"<<< {json.dumps(resp, indent=2, ensure_ascii=False)}", flush=True)
        return resp

    # model -> one did that runs it. The firmware history endpoint is keyed on a
    # device, but the history itself is a property of the model, so one
    # representative device per model is enough.
    models, dids = list(args.model), {}

    if args.all_models:
        found = {}
        for d in list_devices(connector, args.server):
            found.setdefault(d.get("model"), []).append(d.get("name"))
            dids.setdefault(d.get("model"), d.get("did"))
        print(f"\n--- {len(found)} distinct models on the account ---")
        for model, names in sorted(found.items()):
            print(f"  {model:<34} {', '.join(n for n in names if n)}")
        models.extend(m for m in found if m and m not in models)

    if args.ip:
        device = find_device(connector, args.server, args.ip)
        if not device:
            print(f"no device with local address {args.ip} on server "
                  f"'{args.server}'", file=sys.stderr)
            return 1

        did, model = device["did"], device.get("model")
        pid = device.get("pid", 0)
        print(f"\ntarget: {device.get('name')}  model={model}  did={did}  pid={pid}")
        print("--- device_info as the cloud holds it ---")
        print(json.dumps({k: v for k, v in device.items() if k != "token"},
                         indent=2, ensure_ascii=False))
        if model:
            dids.setdefault(model, did)
            if model not in models:
                models.append(model)

        for path, payload in (
            ("/home/checkversion", {"did": did, "pid": pid}),
            ("/v2/device/check_device_version", {"did": did, "pid": pid}),
            ("/home/multi_checkversion", {"dids": [did]}),
            ("/v2/device/latest_ver", {"did": did}),
            ("/v2/device/get_auto_upgrade_config", {"did": did}),
            ("/device/deviceinfo", {"did": did}),
        ):
            probe(path, payload)

        # miIO.info over the cloud relay, for the version the device reports itself.
        probe(f"/home/rpc/{did}", {"id": 1, "method": "miIO.info", "params": []})

    # The endpoint is documented as returning "the latest version" and takes no
    # documented version argument, so if an older build can be requested at all,
    # the key name is a guess. Try the plausible spellings and let the response
    # decide: success is the server echoing back the version we asked for.
    VERSION_KEYS = ("version", "ver", "fw_ver", "firmware_version")

    catalogue = {}
    quiet = not args.raw
    if quiet and len(models) > 1:
        print(f"\n--- sweeping {len(models)} models ---")
        print(f"{'model':<30} {'version':<14} {'size':>9}  chip / verdict")

    for i, spec in enumerate(models):
        if i:
            time.sleep(args.throttle)
        model, _, want = spec.partition("@")
        want = want or args.version

        matched_key = None
        if want:
            for key in VERSION_KEYS:
                latest = ((probe("/home/latest_version", {"model": model, key: want},
                                 quiet=quiet) or {}).get("result")) or {}
                if latest.get("version") == want:
                    matched_key = key
                    break
                time.sleep(args.throttle)
        else:
            latest = ((probe("/home/latest_version", {"model": model},
                             quiet=quiet) or {}).get("result")) or {}

        history = None
        if model in dids and not want:
            resp = probe("/v2/device/get_firmware_history", {"did": dids[model]},
                         quiet=quiet)
            history = ((resp or {}).get("result") or {}).get("list")

        url, md5 = latest.get("safe_url"), latest.get("md5")
        version = latest.get("version")

        chip, size = None, None
        probe_url = url and (not want or matched_key)
        if probe_url and args.esp_only:
            chip, size = identify_chip(url)

        if not url:
            verdict = "no firmware offered"
        elif want and not matched_key:
            verdict = f"version not served - got {version} instead"
        elif want:
            verdict = f"served via key '{matched_key}'" + (f" - {chip}" if chip else "")
        elif args.esp_only and chip is None:
            verdict = "not an ESP-IDF image - skipped"
        elif args.esp_only and str(chip).startswith("probe failed"):
            verdict = chip
        else:
            verdict = chip or "not identified (--esp-only off)"

        if quiet and len(models) > 1:
            print(f"{spec:<30} {str(version or '-'):<14} "
                  f"{(size or 0):>9}  {verdict}", flush=True)

        # Deliberately no dids and no device names: the changelog is about models,
        # and keeping it free of anything account-specific makes it publishable.
        if not want:
            catalogue[model] = {
                "version": version, "md5": md5, "size": size, "chip": chip,
                "upload_time": latest.get("upload_time"),
                "changeLog": latest.get("changeLog"),
                "fw_change_log": latest.get("fw_change_log"),
                "mcu_change_log": latest.get("mcu_change_log"),
                "history": history,
            }

        if not args.download or not url:
            continue
        if want and not matched_key:
            continue          # would just re-download the latest under a wrong name
        if args.esp_only and (chip is None or str(chip).startswith("probe failed")):
            continue
        try:
            with urllib.request.urlopen(url, timeout=90) as r:
                blob = r.read()
        except Exception as e:
            print(f"!! download failed for {model}: {e}", file=sys.stderr)
            continue
        got = hashlib.md5(blob).hexdigest()
        if got != md5:
            print(f"!! md5 mismatch for {model}: got {got}, expected {md5}",
                  file=sys.stderr)
            continue
        out = pathlib.Path(args.download) / f"{model}-stock-{version}.bin"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(blob)
        if not want:
            catalogue[model]["file"] = out.name
            catalogue[model]["size"] = len(blob)
        print(f"    saved {out.name} ({len(blob)} bytes, md5 verified)", flush=True)

    if args.download and catalogue:
        cl = pathlib.Path(args.download) / "changelog.json"
        cl.parent.mkdir(parents=True, exist_ok=True)
        # Merge rather than overwrite. History is only available for models with a
        # device on the account, so a later run over borrowed model names would
        # otherwise throw away the history collected for the ones we own.
        merged = {}
        if cl.is_file():
            merged = json.loads(cl.read_text())
        for model, entry in catalogue.items():
            if entry.get("history") is None and model in merged:
                entry["history"] = merged[model].get("history")
            merged[model] = entry
        cl.write_text(json.dumps(dict(sorted(merged.items())),
                                 indent=2, ensure_ascii=False) + "\n")
        print(f"\nwrote {cl} ({len(merged)} models)")

    return 0


if __name__ == "__main__":
    sys.exit(main())
