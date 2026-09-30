#!/usr/bin/env python3
"""Read the inactive OTA slot off a probe-firmware lamp, over the network.

The lamp runs the probe build in one slot; the vendor's stock image is still in
the other, because a flash always targets the inactive slot and only one flash
has happened. `dump_flash(offset, length)` on the probe reads that slot and logs
the bytes as base64, so the image can be recovered with no UART and no teardown -
which is what makes it usable on a device still under warranty.

Restartable by construction: the output file is preallocated to the partition
size and a sidecar records which offsets have landed, so an interrupted run
picks up where it stopped rather than starting over.

Usage:
    dump_stock.py <ip> <out.bin> [--size BYTES] [--chunk BYTES]
"""

import argparse
import asyncio
import base64
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

ANSI = re.compile(r"\x1b\[[0-9;]*m")
CHUNK_LINE = re.compile(r"CHUNK (\d+) (\d+) (\S+)")

PART_SIZE = 0x1E0000          # miio_fw1 / miio_fw2 on lamp9
CHUNK = 2048                  # MAX_CHUNK in probe_flash.h


async def run(ip: str, out: pathlib.Path, size: int, chunk: int) -> int:
    from aioesphomeapi import APIClient, LogLevel

    done_path = out.with_suffix(out.suffix + ".done")
    done = set()
    if out.is_file() and done_path.is_file():
        done = {int(x) for x in done_path.read_text().split() if x}
        print(f"resuming: {len(done)} chunks already stored")
    else:
        out.write_bytes(b"\x00" * size)
        done_path.write_text("")

    fh = out.open("r+b")
    dfh = done_path.open("a")

    cli = APIClient(ip, 6053, None)
    await cli.connect(login=True)
    try:
        _entities, services = await cli.list_entities_services()
        svc = {s.name: s for s in services}
        if "dump_flash" not in svc:
            raise SystemExit(f"{ip} does not expose dump_flash - is the probe build running?")

        got: dict[int, bytes] = {}

        def on_log(msg):
            m = CHUNK_LINE.search(ANSI.sub("", msg.message.decode(errors="replace")))
            if m:
                got[int(m.group(1))] = base64.b64decode(m.group(3))

        cli.subscribe_logs(on_log, log_level=LogLevel.LOG_LEVEL_DEBUG)
        await asyncio.sleep(1)

        offsets = [o for o in range(0, size, chunk) if o not in done]
        total, failed = len(offsets), []
        for i, off in enumerate(offsets, 1):
            for attempt in range(4):
                got.pop(off, None)
                await cli.execute_service(svc["dump_flash"], {"offset": off, "length": chunk})
                for _ in range(60):                      # up to ~3 s
                    await asyncio.sleep(0.05)
                    if off in got:
                        break
                if off in got:
                    break
                print(f"  retry {attempt + 1} at offset {off}")
            if off not in got:
                failed.append(off)
                continue
            fh.seek(off)
            fh.write(got.pop(off))
            dfh.write(f"{off}\n")
            if i % 50 == 0 or i == total:
                dfh.flush()
                fh.flush()
                print(f"  {i}/{total} chunks  ({off + chunk}/{size} bytes)", flush=True)
        fh.flush()
        dfh.flush()
        if failed:
            print(f"FAILED offsets ({len(failed)}): {failed[:20]}")
        return len(failed)
    finally:
        await cli.disconnect()
        fh.close()
        dfh.close()


if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("ip")
    p.add_argument("out")
    p.add_argument("--size", type=lambda x: int(x, 0), default=PART_SIZE)
    p.add_argument("--chunk", type=lambda x: int(x, 0), default=CHUNK)
    a = p.parse_args()
    sys.exit(asyncio.run(run(a.ip, pathlib.Path(a.out), a.size, a.chunk)))
