#!/bin/sh
# Build src/armv7 and boot it on QEMU's virt board.
#   labs/armv7/qemu/launch.sh
# The UART prints the machine lines and then stays at the shell.
# ls, echo, and sleep are programs on the disk. exit powers off.
# The editor's ARMv7 session sets QEMU_GDB=1 and pauses at reset.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)

if [ -n "${1:-}" ]; then
	echo "Usage: labs/armv7/qemu/launch.sh" >&2
	exit 1
fi

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
	echo "arm-none-eabi-gcc is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v qemu-system-arm >/dev/null 2>&1; then
	echo "qemu-system-arm is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
	echo "python3 is not installed." >&2
	exit 1
fi

make -C "$root/src/armv7"
mkdir -p "$root/images"
python3 "$root/labs/rv32/qemu/mkimage.py" "$root/images/armv7.img" \
	ls "$root/src/armv7/user/ls.elf" \
	sh "$root/src/armv7/user/sh.elf" \
	echo "$root/src/armv7/user/echo.elf" \
	sleep "$root/src/armv7/user/sleep.elf"

disk="-drive file=$root/images/armv7.img,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0,bus=virtio-mmio-bus.0"

if [ -n "${QEMU_GDB:-}" ]; then
	# shellcheck disable=SC2086
	exec qemu-system-arm \
		-machine virt -cpu cortex-a15 -smp 1 \
		-display none -serial stdio \
		-gdb tcp:127.0.0.1:1234 -S \
		$disk \
		-kernel "$root/src/armv7/kernel.bin"
fi

# shellcheck disable=SC2086
exec qemu-system-arm \
	-machine virt -cpu cortex-a15 -smp 1 \
	-display none -serial stdio \
	$disk \
	-kernel "$root/src/armv7/kernel.bin"
