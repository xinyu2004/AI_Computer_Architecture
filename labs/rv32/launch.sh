#!/bin/sh
# Build src/rv32 and boot it on QEMU's virt board.
#   labs/rv32/launch.sh
# The console is the UART named by the device tree. This script does not
# start the 80386 session: F5 and labs/qemu/launch.sh stay as they are.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

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

make -C "$root/src/rv32"

log=$(mktemp)
trap 'rm -f "$log"' EXIT
set +e
if command -v timeout >/dev/null 2>&1; then
	timeout 20 qemu-system-riscv32 \
		-machine virt -cpu rv32 -smp 1 \
		-bios default -display none -serial stdio \
		-kernel "$root/src/rv32/kernel.elf" >"$log" 2>&1
else
	qemu-system-riscv32 \
		-machine virt -cpu rv32 -smp 1 \
		-bios default -display none -serial stdio \
		-kernel "$root/src/rv32/kernel.elf" >"$log" 2>&1
fi
status=$?
set -e
cat "$log"
if [ "$status" -ne 0 ]; then
	echo "qemu-system-riscv32 exited $status" >&2
	exit "$status"
fi

missing=0
for line in '^rv32' '^hart ' '^uart 0x' '^timebase ' '^sv32 on' '^timer' \
	'^task 1' '^copy_page 4096' '^task 0'; do
	if ! grep -q "$line" "$log"; then
		echo "missing line: $line" >&2
		missing=1
	fi
done
exit "$missing"
