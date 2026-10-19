#!/bin/sh
# Build src/aarch64 and boot it on QEMU's virt board.
#   labs/aarch64/qemu/launch.sh
# The UART prints the machine lines and then stays at the shell.
# ls, echo, and sleep are programs on the disk. exit powers off.
# The editor's AArch64 session sets QEMU_GDB=1 and pauses at reset.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)

if [ -n "${1:-}" ]; then
	echo "Usage: labs/aarch64/qemu/launch.sh" >&2
	exit 1
fi

if ! command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then
	echo "aarch64-linux-gnu-gcc is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v qemu-system-aarch64 >/dev/null 2>&1; then
	echo "qemu-system-aarch64 is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
	echo "python3 is not installed." >&2
	exit 1
fi

make -C "$root/src/aarch64"
mkdir -p "$root/images"
python3 "$root/labs/rv32/qemu/mkimage.py" "$root/images/aarch64.img" \
	ls "$root/src/aarch64/user/ls.elf" \
	sh "$root/src/aarch64/user/sh.elf" \
	echo "$root/src/aarch64/user/echo.elf" \
	sleep "$root/src/aarch64/user/sleep.elf"

disk="-drive file=$root/images/aarch64.img,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0,bus=virtio-mmio-bus.0"

if [ -n "${QEMU_GDB:-}" ]; then
	# shellcheck disable=SC2086
	exec qemu-system-aarch64 \
		-machine virt -cpu cortex-a57 -smp 1 \
		-display none -serial stdio \
		-gdb tcp:127.0.0.1:1234 -S \
		$disk \
		-kernel "$root/src/aarch64/kernel.bin"
fi

# shellcheck disable=SC2086
exec qemu-system-aarch64 \
	-machine virt -cpu cortex-a57 -smp 1 \
	-display none -serial stdio \
	$disk \
	-kernel "$root/src/aarch64/kernel.bin"
