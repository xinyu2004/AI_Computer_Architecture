#!/bin/sh
# Boot the step-2 floppy image in QEMU. Run from anywhere:
#   labs/qemu/launch.sh
# The console is the QEMU window. The kernel writes VGA memory, so this
# script does not pass -nographic.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

if ! command -v qemu-system-i386 >/dev/null 2>&1; then
	echo "qemu-system-i386 is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if [ ! -f "$root/src/linux-0.11/Image" ]; then
	echo "Build the floppy image first: make -C src/linux-0.11" >&2
	exit 1
fi
if [ ! -f "$root/rootfs/hdc-0.11.img" ]; then
	echo "Missing rootfs/hdc-0.11.img. See docs/en/build-and-qemu.md." >&2
	exit 1
fi

exec qemu-system-i386 -m 16M -boot a -nic none \
	-drive "file=$root/src/linux-0.11/Image,if=floppy,format=raw" \
	-drive "file=$root/rootfs/hdc-0.11.img,if=ide,format=raw,index=0"
