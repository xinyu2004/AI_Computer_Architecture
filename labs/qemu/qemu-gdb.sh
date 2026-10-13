#!/bin/sh
# Started by the "qemu-gdb" task in .vscode/tasks.json when the
# Linux 0.11 debug session begins. This is not a separate lab command.
# labs/qemu/launch.sh without this task still boots straight to the shell.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)

if [ ! -f "$root/src/linux-0.11/boot/bootsect.elf" ]; then
	echo "Rebuild so debug info exists: make -C src/linux-0.11" >&2
	exit 1
fi
if ! command -v gdb >/dev/null 2>&1; then
	echo "gdb is not installed. Run script/bootstrap.sh." >&2
	exit 1
fi

echo "Display: starting QEMU"
QEMU_GDB=1 "$root/labs/qemu/launch.sh" &
pid=$!

i=0
while [ "$i" -lt 50 ]; do
	if ss -ltn 2>/dev/null | grep -q '127\.0\.0\.1:1234'; then
		echo "GDB listening on 127.0.0.1:1234"
		wait "$pid"
		exit $?
	fi
	if ! kill -0 "$pid" 2>/dev/null; then
		wait "$pid" || true
		echo "QEMU did not stay up. If the disk is locked, close the QEMU window that already has it open." >&2
		exit 1
	fi
	i=$((i + 1))
	sleep 0.1
done

echo "QEMU did not open 127.0.0.1:1234." >&2
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
exit 1
