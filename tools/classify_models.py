#!/usr/bin/env python3
"""Classify Xiaomi models by radio protocol from the public product catalogue.

ESPHome targets ESP32 and ESP8266 (plus RP2040, LibreTiny BK72xx/RTL87xx and
experimental nRF52). It is not a firmware for the Telink and Silicon Labs parts
used in Zigbee and BLE-mesh lights, so those models are not conversion
candidates whatever their firmware looks like.

Filtering them out here rather than by probing firmware matters because the
Xiaomi firmware endpoint is rate limited and sits behind an account login, while
this catalogue is a public community mirror. Nothing in this script touches
Xiaomi's API or needs credentials.

home.miot-spec.com/p/<model> carries a Protocol attribute in its embedded JSON:

    {"name":"Protocol","value":"Wi‑Fi"}

Note the value uses a non-breaking hyphen (U+2011), not an ASCII one. Observed
values include Wi-Fi, MiIO, Bluetooth, BLE Mesh, Sub-device and IR; only the
Wi-Fi and MiIO ones can plausibly be an ESP part.

Responses are cached per model, so re-runs and interrupted runs cost nothing.

Usage:
    classify_models.py --models-from models.txt --out protocols.json
"""

import argparse
import html
import json
import pathlib
import re
import sys
import time
import urllib.error
import urllib.request

CATALOGUE = "https://home.miot-spec.com/p/{model}"
UA = "Mozilla/5.0 (compatible; firmware-inventory/1.0)"

# Protocols that could be an ESP part. Everything else is a different radio and
# therefore a different SoC family.
ESP_CAPABLE = {"wi-fi", "wifi", "miio"}


def normalise(protocol: str) -> str:
    """U+2011 non-breaking hyphen -> ASCII, so 'Wi‑Fi' compares as 'wi-fi'."""
    return protocol.replace("‑", "-").replace("‐", "-").strip().lower()


def fetch(model: str, cache: pathlib.Path) -> str | None:
    cache.mkdir(parents=True, exist_ok=True)
    cached = cache / f"{model}.html"
    if cached.is_file():
        return cached.read_text(errors="replace")
    req = urllib.request.Request(CATALOGUE.format(model=model),
                                 headers={"User-Agent": UA})
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            body = r.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        if e.code == 404:
            cached.write_text("")          # cache the miss too
            return ""
        print(f"  {model}: HTTP {e.code}", file=sys.stderr)
        return None
    except Exception as e:
        print(f"  {model}: {e}", file=sys.stderr)
        return None
    cached.write_text(body)
    return body


def parse(model: str, body: str) -> dict:
    protocol = re.search(r'"name":"Protocol","value":"([^"]*)"', body)
    name = re.search(r'"name":"([^"]*)","model":"' + re.escape(model) + '"', body)
    feature = re.search(r'"name":"Feature","value":"([^"]*)"', body)
    return {
        "name": html.unescape(name.group(1)) if name else None,
        "protocol": html.unescape(protocol.group(1)) if protocol else None,
        "feature": html.unescape(feature.group(1)) if feature else None,
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--models-from", required=True, metavar="FILE",
                   help="model names, one per line (# comments ok)")
    p.add_argument("--out", default="protocols.json", metavar="FILE")
    p.add_argument("--cache", default=".catalogue-cache", metavar="DIR")
    p.add_argument("--throttle", type=float, default=0.4, metavar="SEC")
    args = p.parse_args()

    models = []
    for line in pathlib.Path(args.models_from).read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if line:
            models.append(line)

    cache = pathlib.Path(args.cache)
    out = {}
    for i, model in enumerate(models):
        was_cached = (cache / f"{model}.html").is_file()
        if i and not was_cached:
            time.sleep(args.throttle)
        body = fetch(model, cache)
        if body is None:
            continue
        info = parse(model, body) if body else {"name": None, "protocol": None,
                                               "feature": None}
        info["esp_capable"] = normalise(info["protocol"] or "") in ESP_CAPABLE
        out[model] = info
        print(f"{model:<30} {str(info['protocol'] or 'not in catalogue'):<14} "
              f"{'ESP-capable' if info['esp_capable'] else '-':<12} "
              f"{info['name'] or ''}", flush=True)

    pathlib.Path(args.out).write_text(
        json.dumps(dict(sorted(out.items())), indent=2, ensure_ascii=False) + "\n")
    n = sum(1 for v in out.values() if v["esp_capable"])
    print(f"\nwrote {args.out}: {len(out)} models, {n} ESP-capable")
    return 0


if __name__ == "__main__":
    sys.exit(main())
