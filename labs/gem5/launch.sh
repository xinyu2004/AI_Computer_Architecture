#!/bin/sh
# Run one Linux 0.11 path in gem5 and print cycles, L1 misses,
# and one pipeline stall. Run from anywhere:
#   labs/gem5/launch.sh
#   labs/gem5/launch.sh timer
#   labs/gem5/launch.sh schedule
#   labs/gem5/launch.sh copy_page
# This does not boot the floppy. QEMU and Bochs stay as they are.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
cd "$root"

elf=src/linux-0.11/tools/system.elf
gem5=.gem5/build/X86/gem5.opt

if [ ! -f "$elf" ]; then
	echo "Build the kernel first: make -C src/linux-0.11" >&2
	exit 1
fi
if [ ! -x "$gem5" ]; then
	echo "Build gem5 first: labs/gem5/build.sh" >&2
	exit 1
fi

mode=${1:-paging}
case "$mode" in
paging)
	symbol=setup_paging
	done_pat="setup_paging returned"
	extra=
	# 5e9 ticks is 5 million cycles at 1 GHz (1 tick = 1 ps).
	max_tick=5000000000
	;;
timer)
	symbol=_timer_interrupt
	done_pat="timer_interrupt returned"
	extra="--timer-interrupt --count timer"
	# The PIT fires about 10ms after the kernel programs it.
	max_tick=100000000000
	;;
schedule)
	symbol=_schedule
	done_pat="schedule switched|schedule returned"
	extra="--timer-interrupt --count schedule"
	# fork's schedule() runs well before the 10ms timer tick.
	max_tick=5000000000
	;;
copy_page)
	symbol=copy_page
	done_pat="copy_page returned"
	extra="--timer-interrupt --count copy_page"
	max_tick=5000000000
	;;
*)
	echo "Usage: labs/gem5/launch.sh [paging|timer|schedule|copy_page]" >&2
	exit 1
	;;
esac

if [ "$mode" = copy_page ]; then
	# copy_page is a macro. The rep movsl is inlined in _un_wp_page.
	start=$(nm "$elf" | awk '$3 == "_un_wp_page" { print $1; exit }')
	stop=$(nm "$elf" | awk '$3 == "_do_wp_page" { print $1; exit }')
	raw=$(objdump -d --start-address="0x$start" --stop-address="0x$stop" "$elf" |
		awk '/rep movsl/ { print $1; exit }')
	addr=$(printf '%s' "$raw" | tr -d ':')
	if [ -z "$addr" ]; then
		echo "system.elf has no rep movsl in _un_wp_page." >&2
		exit 1
	fi
else
	addr=$(nm "$elf" | awk -v sym="$symbol" '$3 == sym { print $1; exit }')
	if [ -z "$addr" ]; then
		echo "system.elf has no $symbol symbol." >&2
		exit 1
	fi
fi
# nm prints the address without a 0x prefix. Strip leading zeros for display.
printf '%s at 0x%x\n' "$symbol" "0x$addr"

export GEM5_ROOT="$root/.gem5"
mkdir -p m5out
log=m5out/gem5.log
if ! "$gem5" --outdir=m5out \
	labs/gem5/linux-0.11-x86.py \
	--kernel="$root/$elf" \
	--cpu-type=X86O3CPU \
	--caches \
	--l1d_size=32KiB \
	--l1i_size=32KiB \
	--mem-size=16MiB \
	--cpu-clock=1GHz \
	--sys-clock=1GHz \
	--rel-max-tick="$max_tick" \
	$extra >"$log" 2>&1; then
	cat "$log"
	echo "gem5 failed. See $log" >&2
	exit 1
fi
if ! grep -E -q "$done_pat" "$log"; then
	cat "$log"
	echo "gem5 did not report that $symbol finished. See $log" >&2
	exit 1
fi

stats=m5out/stats.txt
field() {
	val=$(awk -v name="$1" '$1 == name { print $2; exit }' "$stats")
	# gem5 omits a counter that stayed zero.
	if [ -z "$val" ]; then
		val=0
	fi
	printf '%s' "$val"
}
cycles=$(field system.cpu.numCycles)
insts=$(field system.cpu.commitStats0.numInsts)
ops=$(field system.cpu.commitStats0.numOps)
imiss=$(field system.cpu.icache.overallMisses::total)
dmiss=$(field system.cpu.dcache.overallMisses::total)
stall=$(field system.cpu.rename.SQFullEvents)
serial=$(field "system.cpu.rename.status::SerializeStall")
if [ "$cycles" = 0 ] || [ "$insts" = 0 ]; then
	echo "stats.txt is missing cycles or instructions. See $stats" >&2
	exit 1
fi

printf 'cycles              %8s   system.cpu.numCycles\n' "$cycles"
printf 'instructions        %8s   system.cpu.commitStats0.numInsts\n' "$insts"
printf 'microops            %8s   system.cpu.commitStats0.numOps\n' "$ops"
printf 'L1 I misses         %8s   system.cpu.icache.overallMisses::total\n' "$imiss"
printf 'L1 D misses         %8s   system.cpu.dcache.overallMisses::total\n' "$dmiss"
printf 'store-queue stalls  %8s   system.cpu.rename.SQFullEvents\n' "$stall"
printf 'serialize stalls    %8s   system.cpu.rename.status::SerializeStall\n' "$serial"
echo "Model: gem5 O3 at 1 GHz, 32KiB L1 I and D. Full log: m5out/gem5.log"
