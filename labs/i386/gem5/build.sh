#!/bin/sh
# Build the pinned gem5 used by step 4. Run from anywhere:
#   labs/i386/gem5/build.sh
# gem5 itself is not a distro package. This script clones v25.1.0.1
# into .gem5/, applies the Linux 0.11 x86 patch, and compiles
# build/X86/gem5.opt. scons comes from pip.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
cd "$root"

if ! command -v python3 >/dev/null 2>&1; then
	echo "python3 is missing." >&2
	exit 1
fi
if ! command -v m4 >/dev/null 2>&1; then
	echo "m4 is missing. Run script/bootstrap.sh." >&2
	exit 1
fi
if ! python3 -c 'import sys; raise SystemExit(0 if sys.version_info >= (3, 6) else 1)'; then
	echo "python3 3.6 or newer is required." >&2
	exit 1
fi

if [ ! -f .gem5/SConstruct ]; then
	echo "Cloning gem5 v25.1.0.1 into .gem5/"
	git clone --branch v25.1.0.1 --depth 1 https://github.com/gem5/gem5.git .gem5
fi

if ! grep -q initI386Protected .gem5/src/arch/x86/fs_workload.cc; then
	echo "Applying labs/i386/gem5/linux-0.11-x86.patch"
	patch -p1 -d .gem5 < labs/i386/gem5/linux-0.11-x86.patch
fi

if [ ! -x .gem5/.venv/bin/python ]; then
	echo "Creating .gem5/.venv"
	python3 -m venv .gem5/.venv
fi
if ! .gem5/.venv/bin/python -c 'import SCons' >/dev/null 2>&1; then
	echo "Installing scons into .gem5/.venv"
	.gem5/.venv/bin/python -m pip install scons
fi

# gem5 embeds the system Python. Point it at that interpreter's config
# even though scons itself runs from the venv.
if [ -z "${PYTHON_CONFIG:-}" ]; then
	PYTHON_CONFIG=$(command -v python3-config || true)
	if [ -z "$PYTHON_CONFIG" ]; then
		echo "python3-config is missing. Install python3-dev." >&2
		exit 1
	fi
	export PYTHON_CONFIG
fi

echo "Building .gem5/build/X86/gem5.opt"
cd .gem5
exec .venv/bin/python -m SCons -j"$(nproc)" build/X86/gem5.opt
