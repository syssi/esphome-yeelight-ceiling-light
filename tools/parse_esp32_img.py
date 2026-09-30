#!/usr/bin/env python3
"""Parse an ESP-IDF app image; report structure and any trailing bytes past the
end of the legitimate image (i.e. whatever Xiaomi's 'crc' step appends)."""
import binascii
import hashlib
import struct
import sys

path = sys.argv[1]
data = open(path, "rb").read()
print(f"file: {path}")
print(f"size: {len(data)} bytes")

if data[0] != 0xE9:
    print(f"!! first byte 0x{data[0]:02x}, not 0xE9 — not a raw ESP-IDF image")
    sys.exit(0)

seg_count = data[1]
entry = struct.unpack("<I", data[4:8])[0]
chip_id = struct.unpack("<H", data[12:14])[0]
hash_appended = data[23]
print(f"header: segments={seg_count} entry=0x{entry:08x} chip_id={chip_id} "
      f"hash_appended={hash_appended}")

off = 24
for i in range(seg_count):
    load_addr, seg_len = struct.unpack("<II", data[off:off + 8])
    off += 8 + seg_len

pad = 15 - (off % 16)
off += pad + 1
if hash_appended == 1:
    sha_off = off
    stored = binascii.hexlify(data[sha_off:sha_off + 32]).decode()
    calc = hashlib.sha256(data[:sha_off]).hexdigest()
    off += 32
    print(f"SHA-256 stored : {stored}")
    print(f"SHA-256 calc   : {calc}")
    print(f"SHA-256 {'MATCHES' if stored == calc else 'MISMATCH'}")

print(f"legitimate image ends at {off} (0x{off:x})")
trailer = data[off:]
print(f"trailing bytes: {len(trailer)}" + (f"  hex={binascii.hexlify(trailer).decode()}" if trailer else "  (none)"))
