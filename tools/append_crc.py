#!/usr/bin/env python3
"""Append Xiaomi's 4-byte OTA trailer to an ESP-IDF application image.

Xiaomi's ESP32 firmware images are a normal ESP-IDF application image followed by
four extra bytes. An ESPHome build has no trailer, and the stock updater will not
accept it without one.

The algorithm was recovered by matching against genuine Xiaomi images. It is not
a standard CRC-32 - none of the catalogued variants reproduce it:

    CRC-32, polynomial 0x04C11DB7 (reflected 0xEDB88320)
            init 0x00000000, refin/refout = true, xorout 0x00000000
            stored little-endian

i.e. standard CRC-32 without the customary pre- and post-inversion. Verified
against an ESP32 image and an MT7697 image from different Xiaomi product lines;
both reproduce exactly.

Usage:
    append_crc.py <in.bin> <out.bin>       append the trailer
    append_crc.py --verify <image.bin>     check a genuine Xiaomi image
"""

import struct
import sys

_TABLE = []
for _i in range(256):
    _c = _i
    for _ in range(8):
        _c = (_c >> 1) ^ (0xEDB88320 if _c & 1 else 0)
    _TABLE.append(_c)


def xiaomi_crc(buf: bytes) -> int:
    """CRC-32 with a zero init and no final inversion."""
    crc = 0
    for b in buf:
        crc = (crc >> 8) ^ _TABLE[(crc ^ b) & 0xFF]
    return crc & 0xFFFFFFFF


def verify(path: str) -> int:
    with open(path, "rb") as f:
        data = f.read()
    body, trailer = data[:-4], data[-4:]
    packed = struct.pack("<I", xiaomi_crc(body))
    print(f"stored trailer : {trailer.hex()}")
    print(f"computed (LE)  : {packed.hex()}")
    ok = packed == trailer
    print("VERIFIED" if ok else "MISMATCH")
    return 0 if ok else 1


def append(src: str, dst: str) -> int:
    with open(src, "rb") as f:
        body = f.read()
    if body[:1] != b"\xe9":
        print(f"warning: {src} does not start with 0xE9 and may not be an "
              f"ESP-IDF application image", file=sys.stderr)
    crc = xiaomi_crc(body)
    out = body + struct.pack("<I", crc)
    with open(dst, "wb") as f:
        f.write(out)
    print(f"in   : {src}  {len(body)} bytes")
    print(f"crc  : {crc:08x}  -> trailer {struct.pack('<I', crc).hex()}")
    print(f"out  : {dst}  {len(out)} bytes")
    return 0


def main() -> int:
    if len(sys.argv) == 3 and sys.argv[1] == "--verify":
        return verify(sys.argv[2])
    if len(sys.argv) == 3:
        return append(sys.argv[1], sys.argv[2])
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
