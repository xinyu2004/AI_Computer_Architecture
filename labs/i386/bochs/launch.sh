#!/bin/sh
# Open the step-2 disks in Bochs. Run from anywhere:
#   labs/i386/bochs/launch.sh
#   labs/i386/bochs/launch.sh vnc
# The debugger prompt stays in this terminal. A local display opens an
# SDL window. With no display, or with the vnc argument, Bochs listens
# for a VNC client and the debugger prompt appears after the client connects.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
cd "$root"

if ! command -v bochs >/dev/null 2>&1; then
	echo "bochs is not installed. Run script/bootstrap.sh." >&2
	echo "What that installs, and why: docs/en/tools.md" >&2
	exit 1
fi
if [ ! -f src/linux-0.11/Image ]; then
	echo "Build the floppy image first: make -C src/linux-0.11" >&2
	exit 1
fi
if [ ! -f rootfs/hdc-0.11.img ]; then
	echo "Missing rootfs/hdc-0.11.img. See docs/en/build-and-qemu.md." >&2
	exit 1
fi
if [ ! -f src/linux-0.11/boot/setup.elf ] || [ ! -f src/linux-0.11/tools/system.elf ]; then
	echo "Build the ELF files the addresses come from: make -C src/linux-0.11" >&2
	exit 1
fi

if [ "$#" -gt 1 ]; then
	echo "Usage: labs/i386/bochs/launch.sh [sdl|vnc]" >&2
	exit 1
fi

case ${1:-} in
sdl|vnc) mode=$1 ;;
'')
	if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
		mode=vnc
	else
		mode=sdl
	fi
	;;
*)
	echo "Usage: labs/i386/bochs/launch.sh [sdl|vnc]" >&2
	exit 1
	;;
esac

case $mode in
sdl)
	echo "Display: SDL window. The debugger prompt stays in this terminal." >&2
	display='display_library: sdl2'
	;;
vnc)
	cat >&2 <<'EOF'
Display: VNC. Bochs prints the port it opened, usually 5900.
The debugger prompt appears in this terminal after a client connects.
From the computer you are sitting at, forward that port:

  ssh -L 5900:127.0.0.1:5900 <user>@<this-host>

Then open a VNC client on localhost:5900.
EOF
	display='display_library: rfb, options=timeout=120'
	;;
esac

lmsw_off=$(objdump -d -m i8086 src/linux-0.11/boot/setup.elf | awk '/lmsw/ { gsub(":", "", $1); print $1; exit }')
if [ -z "$lmsw_off" ]; then
	echo "Could not find lmsw in src/linux-0.11/boot/setup.elf." >&2
	exit 1
fi
lmsw=$(printf '0x%x' $((0x90200 + 0x$lmsw_off)))

sym() {
	nm src/linux-0.11/tools/system.elf | awk -v name="$1" '$3 == name { print $1; exit }'
}

paging=$(sym setup_paging)
timer=$(sym _timer_interrupt)
schedule=$(sym _schedule)
if [ -z "$paging" ] || [ -z "$timer" ] || [ -z "$schedule" ]; then
	echo "Could not read setup_paging, _timer_interrupt, or _schedule from tools/system.elf." >&2
	exit 1
fi
paging=$(printf '0x%x' "0x$paging")
timer=$(printf '0x%x' "0x$timer")
schedule=$(printf '0x%x' "0x$schedule")

cat <<EOF
Debugger commands for this build. The kernel addresses move every time you link.

Protected mode, lmsw at $lmsw
  pb $lmsw
  c
  s
  s
  creg
  sreg
  info gdt 0 2

Paging, setup_paging at $paging
  pb $paging
  c
  creg

Timer, _timer_interrupt at $timer
  pb $timer
  c
  info idt 0x20

Task switch, _schedule at $schedule
  pb $schedule
  c
  info tss

q quits. help lists the debugger commands.
EOF

tmp=$(mktemp)
cat labs/i386/bochs/bochsrc > "$tmp"
printf '%s\n' "$display" >> "$tmp"
exec bochs -q -f "$tmp"
