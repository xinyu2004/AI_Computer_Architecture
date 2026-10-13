#!/bin/sh
# Install the host tools for the steps that are ready.
# Why these packages, and which step each one serves: docs/en/tools.md

set -e

cat <<'EOF'
script/bootstrap.sh installs the host tools for the steps that are ready.
Each package belongs to one step.

Step 2, build the kernel
  build-essential    make, as, ld       turn src/linux-0.11/ into Image
  gcc-multilib       gcc -m32           this kernel is 32-bit 80386 code

Step 2, run it
  qemu-system-x86    qemu-system-i386   labs/qemu/launch.sh opens this emulator
  gdb                gdb                F5, Linux 0.11, stops on the first instruction

Step 3, machine state
  bochs              bochs              labs/bochs/launch.sh, internal debugger
  bochs-sdl          SDL window         the VGA console when a display is available
  bochsbios          BIOS-bochs-legacy  firmware for that PC
  vgabios            vgabios.bin        VGA firmware

Step 4, cycles and misses
  python3-dev        Python.h           gem5 embeds this Python
  python3-venv       python3 -m venv    scons is installed in .gem5/.venv
  m4                 m4                 the gem5 build runs m4

gem5 itself is not an apt package. labs/gem5/build.sh clones v25.1.0.1
into .gem5/ and compiles it. See docs/en/tools.md.
EOF

if [ -r /etc/os-release ]; then
	# shellcheck disable=SC1091
	. /etc/os-release
fi

case " ${ID:-} ${ID_LIKE:-} " in
*" debian "*|*" ubuntu "*)
	;;
*)
	echo "This script installs packages on Debian and Ubuntu." >&2
	echo "Install the commands listed above with the packages on your system." >&2
	exit 1
	;;
esac

sudo apt-get update
sudo apt-get install -y build-essential gcc-multilib qemu-system-x86 gdb \
	bochs bochs-sdl bochsbios vgabios \
	python3-dev python3-venv m4
