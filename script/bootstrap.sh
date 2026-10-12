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

Not installed yet
  Bochs              step 3             machine state
  gem5               step 4             cycles and cache misses

See docs/en/tools.md.
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
sudo apt-get install -y build-essential gcc-multilib qemu-system-x86
