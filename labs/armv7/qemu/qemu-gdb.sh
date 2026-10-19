#!/bin/sh
# Started by the "armv7-gdb" task in .vscode/tasks.json when the
# ARMv7 debug session begins. This is not a separate lab command.
# labs/armv7/qemu/launch.sh without this task still boots to the shell.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)

if ! command -v gdb-multiarch >/dev/null 2>&1; then
	echo "gdb-multiarch is not installed. Run script/bootstrap.sh." >&2
	exit 1
fi

echo "Display: starting QEMU"
QEMU_GDB=1 "$root/labs/armv7/qemu/launch.sh" &
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
		echo "QEMU did not stay up. If 127.0.0.1:1234 is already open, close the other debug session." >&2
		exit 1
	fi
	i=$((i + 1))
	sleep 0.1
done

echo "QEMU did not open 127.0.0.1:1234." >&2
kill "$pid" 2>/dev/null || true
wait "$pid" 2>/dev/null || true
exit 1
