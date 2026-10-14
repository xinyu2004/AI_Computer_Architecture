#!/bin/sh
# Boot the step-2 floppy image in QEMU. Run from anywhere:
#   labs/i386/qemu/launch.sh
#   labs/i386/qemu/launch.sh vnc
# The editor's Linux 0.11 debug session sets QEMU_GDB=1 and runs this
# same script. Without that variable the machine boots straight to the shell.
# The kernel writes VGA memory, so this script does not pass -nographic.
# A local desktop opens a GTK window. SSH X11 forwarding (display number
# 10 or higher) opens an SDL window. With no display, or with the vnc
# argument, QEMU listens on 127.0.0.1:5901.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)

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

screen_of_display() {
	d=${1##*:}
	d=${d%%.*}
	case $d in
	''|*[!0-9]*) return 1 ;;
	esac
	printf '%s\n' "$d"
}

detect_display() {
	if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
		printf '%s\n' vnc
		return
	fi
	if [ -n "${DISPLAY:-}" ]; then
		n=$(screen_of_display "$DISPLAY") || {
			printf '%s\n' gtk
			return
		}
		if [ "$n" -ge 10 ]; then
			printf '%s\n' sdl
			return
		fi
	fi
	printf '%s\n' gtk
}

if [ "$#" -gt 1 ]; then
	echo "Usage: labs/i386/qemu/launch.sh [gtk|sdl|vnc]" >&2
	exit 1
fi

case ${1:-} in
gtk|sdl|vnc) mode=$1 ;;
'') mode=$(detect_display) ;;
*)
	echo "Usage: labs/i386/qemu/launch.sh [gtk|sdl|vnc]" >&2
	exit 1
	;;
esac

case $mode in
gtk)
	echo "Display: GTK window." >&2
	set --
	;;
sdl)
	echo "Display: SDL window (SSH X11 forwarding)." >&2
	set -- -display sdl,gl=off
	;;
vnc)
	cat >&2 <<'EOF'
Display: VNC at 127.0.0.1:5901 on this machine.
From the computer you are sitting at, forward that port:

  ssh -L 5901:127.0.0.1:5901 <user>@<this-host>

Then open a VNC client on localhost:5901.
EOF
	set -- -display vnc=127.0.0.1:1
	;;
esac

if [ -n "${QEMU_GDB:-}" ]; then
	set -- -gdb tcp:127.0.0.1:1234 -S "$@"
fi

exec qemu-system-i386 "$@" -m 16M -boot a -nic none \
	-drive "file=$root/src/linux-0.11/Image,if=floppy,format=raw" \
	-drive "file=$root/rootfs/hdc-0.11.img,if=ide,format=raw,index=0"
