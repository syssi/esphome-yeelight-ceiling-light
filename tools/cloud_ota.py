#!/usr/bin/env python3
"""Relay a miIO OTA command to a Yeelight through the Xiaomi cloud.

Current stock firmware refuses `miIO.ota` sent directly over the LAN:

    {"code": -30020, "message": "service not available."}

The same command is accepted when it arrives from the Xiaomi cloud, and the
`app_url` may point at a private LAN address over plain HTTP. The cloud only
relays the instruction - the firmware file never leaves your network.

    POST https://<region>.api.io.mi.com/app/home/rpc/<did>
    data = {"id":N,"method":"miIO.ota","params":{"app_url":"http://..."}}

Note the parameter shape: `app_url` alone (or `mcu_url` for the companion MCU).
That is the form confirmed to work; the larger payload documented by python-miio
was not tested over this channel. No checksum is passed in this form.

Authentication is handled by Xiaomi-cloud-tokens-extractor, which deals with the
captcha and email 2FA that Xiaomi's login now requires:

    https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor

Point --extractor at a checkout of it. Credentials are read from the environment
so they never appear in a command line or shell history:

    export MI_USERNAME=...
    export MI_PASSWORD=...

Usage:
    cloud_ota.py --ip 192.0.2.10 --url http://192.0.2.2:8000/fw_crc.bin
    cloud_ota.py --ip 192.0.2.10 --state          # just read OTA state
"""

import argparse
import json
import os
import sys
import time


def load_connector(extractor_path: str):
    sys.path.insert(0, extractor_path)
    try:
        import token_extractor as te  # noqa: PLC0415
    except ImportError:
        print(f"could not import token_extractor from {extractor_path}\n"
              f"clone Xiaomi-cloud-tokens-extractor and pass --extractor",
              file=sys.stderr)
        raise SystemExit(2)
    return te


def find_device(connector, server: str, ip: str):
    homes = []
    h = connector.get_homes(server)
    if h:
        homes.extend({"home_id": x["id"], "home_owner": connector.userId}
                     for x in h["result"]["homelist"])
    cnt = connector.get_dev_cnt(server)
    if cnt:
        homes.extend({"home_id": x["home_id"], "home_owner": x["home_owner"]}
                     for x in cnt["result"]["share"]["share_family"])

    for home in homes:
        devs = connector.get_devices(server, home["home_id"], home["home_owner"])
        if not devs or not devs.get("result", {}).get("device_info"):
            continue
        for d in devs["result"]["device_info"]:
            if d.get("localip") == ip:
                return d
    return None


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__,
                                formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--ip", required=True, help="device LAN address, used to identify it")
    p.add_argument("--url", help="firmware URL to hand the device")
    p.add_argument("--server", default="de",
                   help="Xiaomi cloud region: cn, de, us, ru, tw, sg, i2 (default: de)")
    p.add_argument("--extractor", default="./Xiaomi-cloud-tokens-extractor",
                   help="path to a Xiaomi-cloud-tokens-extractor checkout")
    p.add_argument("--state", action="store_true",
                   help="only read OTA state and exit")
    p.add_argument("--poll", type=int, default=40,
                   help="how many 5s polls to run after sending (default: 40)")
    args = p.parse_args()

    if not args.state and not args.url:
        p.error("--url is required unless --state is given")

    for var in ("MI_USERNAME", "MI_PASSWORD"):
        if not os.environ.get(var):
            print(f"{var} is not set", file=sys.stderr)
            return 2

    sys.argv = ["token_extractor.py",
                "-u", os.environ["MI_USERNAME"].strip(),
                "-p", os.environ["MI_PASSWORD"].strip(),
                "-l", "CRITICAL"]
    te = load_connector(args.extractor)

    connector = te.PasswordXiaomiCloudConnector()
    if not connector.login():
        print("login failed", file=sys.stderr)
        return 1

    device = find_device(connector, args.server, args.ip)
    if not device:
        print(f"no device with local address {args.ip} on server "
              f"'{args.server}'", file=sys.stderr)
        return 1

    did = device["did"]
    print(f"target: {device.get('name')}  model={device.get('model')}  did={did}")

    url = connector.get_api_url(args.server) + f"/home/rpc/{did}"
    counter = [0]

    def relay(method, params):
        counter[0] += 1
        data = (f'{{"id":{counter[0]},"method":"{method}",'
                f'"params":{json.dumps(params)}}}')
        print(f">>> {data}", flush=True)
        resp = connector.execute_api_call_encrypted(url, {"data": data})
        print(f"<<< {json.dumps(resp)}\n", flush=True)
        return resp

    relay("miIO.get_ota_state", [])
    relay("miIO.get_ota_progress", [])
    if args.state:
        return 0

    relay("miIO.ota", {"app_url": args.url})

    # `installed` means the image was written; the device then reboots and stops
    # answering miIO entirely, which is the real success signal.
    for i in range(args.poll):
        time.sleep(5)
        r = relay("miIO.get_ota_state", [])
        state = (r or {}).get("result")
        if state and state[0] in ("installing", "installed"):
            print(f"[t+{(i + 1) * 5}s] state = {state[0]}", flush=True)
        if state and state[0] == "failed":
            print("OTA reported failure", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
