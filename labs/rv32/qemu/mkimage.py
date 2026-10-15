#!/usr/bin/env python3
# Build the RV32 root image the kernel mounts.
#   mkimage.py images/rv32.img ls src/user/ls.elf sh src/user/sh.elf
# Sector 0: magic V2SF and the file count. Sector 1: directory entries
# of 28-byte name, offset, length. Each file starts on a sector boundary.

import struct
import sys

MAGIC = 0x46533256
SECTOR = 512
NAME = 28


def main():
    if len(sys.argv) < 4 or len(sys.argv) % 2 != 0:
        sys.stderr.write(
            "usage: mkimage.py OUT name FILE [name FILE ...]\n"
        )
        return 1
    out = sys.argv[1]
    files = []
    args = sys.argv[2:]
    for i in range(0, len(args), 2):
        name = args[i].encode("ascii")
        if len(name) > NAME - 1:
            sys.stderr.write("name too long\n")
            return 1
        with open(args[i + 1], "rb") as f:
            files.append((name, f.read()))
    blob = bytearray(SECTOR * 2)
    struct.pack_into("<II", blob, 0, MAGIC, len(files))
    data_off = SECTOR * 2
    for index, (name, data) in enumerate(files):
        entry = name + b"\0" * (NAME - len(name))
        entry += struct.pack("<II", data_off, len(data))
        at = SECTOR + index * len(entry)
        blob[at : at + len(entry)] = entry
        blob.extend(data)
        pad = (-len(blob)) % SECTOR
        blob.extend(b"\0" * pad)
        data_off = len(blob)
    with open(out, "wb") as f:
        f.write(blob)
    return 0


if __name__ == "__main__":
    sys.exit(main())
