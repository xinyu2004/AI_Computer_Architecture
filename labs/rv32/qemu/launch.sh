#!/bin/sh
# Build src/rv32 and boot it on QEMU's virt board.
#   labs/rv32/qemu/launch.sh
# The UART prints the machine lines and then stays at the shell.
# ls, echo, and sleep are programs on the disk. exit powers off.
# The editor's RV32 session sets QEMU_GDB=1 and pauses at reset.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)

if [ -n "${1:-}" ]; then
	echo "Usage: labs/rv32/qemu/launch.sh" >&2
	exit 1
fi

if ! command -v riscv64-unknown-elf-gcc >/dev/null 2>&1; then
	echo "riscv64-unknown-elf-gcc is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v qemu-system-riscv32 >/dev/null 2>&1; then
	echo "qemu-system-riscv32 is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
	echo "python3 is not installed." >&2
	exit 1
fi

make -C "$root/src/rv32"
mkdir -p "$root/images"
python3 "$root/labs/rv32/qemu/mkimage.py" "$root/images/rv32.img" \
	ls "$root/src/user/ls.elf" \
	sh "$root/src/user/sh.elf" \
	echo "$root/src/user/echo.elf" \
	sleep "$root/src/user/sleep.elf"

disk="-drive file=$root/images/rv32.img,if=none,format=raw,id=hd0 -device virtio-blk-device,drive=hd0"

if [ -n "${QEMU_GDB:-}" ]; then
	# shellcheck disable=SC2086
	exec qemu-system-riscv32 \
		-machine virt -cpu rv32 -smp 1 \
		-bios none -display none -serial stdio \
		-gdb tcp:127.0.0.1:1234 -S \
		$disk \
		-kernel "$root/src/rv32/kernel.elf"
fi

# shellcheck disable=SC2086
exec qemu-system-riscv32 \
	-machine virt -cpu rv32 -smp 1 \
	-bios none -display none -serial stdio \
	$disk \
	-kernel "$root/src/rv32/kernel.elf"
