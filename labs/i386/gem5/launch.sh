#!/bin/sh
# Run one Linux 0.11 path in gem5 and print the counters for that
# window: cycles, CPI, the load/store mix, L1 miss rates, average
# data-cache miss latency, branches, and the rename stalls.
# Also write labs/i386/gem5/m5out/instructions.txt, pipeline.txt, and
# pipeline.html for the same window. timer and schedule keep
# the whole window. paging keeps the first 32 microops and the last
# 12. copy_page keeps the first 32. The HTML file is one colored row
# per microop.
# Run from anywhere:
#   labs/i386/gem5/launch.sh
#   labs/i386/gem5/launch.sh timer
#   labs/i386/gem5/launch.sh schedule
#   labs/i386/gem5/launch.sh copy_page
#   labs/i386/gem5/launch.sh from START to END
# START and END are symbols in tools/system.elf, or hex addresses.
# Both are fixed before the run. END is the first instruction outside
# the window. An end computed while the kernel runs stays a named
# argument (schedule). This does not boot the floppy.

set -e
root=$(CDPATH= cd -- "$(dirname "$0")/../../.." && pwd)
cd "$root"

elf=src/linux-0.11/tools/system.elf
gem5=.gem5/build/X86/gem5.opt

if [ ! -f "$elf" ]; then
	echo "Build the kernel first: make -C src/linux-0.11" >&2
	exit 1
fi
if [ ! -x "$gem5" ]; then
	echo "Build gem5 first: labs/i386/gem5/build.sh" >&2
	exit 1
fi

# A symbol wins over a hex spelling, so "add" is the symbol if nm has it.
resolve_pc() {
	found=$(nm "$elf" | awk -v sym="$1" '$3 == sym { print $1; exit }')
	if [ -n "$found" ]; then
		printf '%s' "$found"
		return
	fi
	printf '%s' "$1" | awk '
		{
			a = $0
			if (a ~ /^0[xX]/)
				a = substr(a, 3)
			if (a ~ /^[0-9A-Fa-f]+$/ && a !~ /^0+$/) {
				print tolower(a)
				ok = 1
			}
		}
		END { exit !ok }
	'
}

mode=${1:-paging}
range_from=
range_to=
if [ "$mode" = from ]; then
	if [ $# -ne 4 ] || [ "$3" != to ]; then
		echo "Usage: labs/i386/gem5/launch.sh from START to END" >&2
		echo "START and END are symbols, or hex addresses." >&2
		exit 1
	fi
	mode=range
	range_from=$(resolve_pc "$2") || {
		echo "No symbol $2 in system.elf, and it is not a hex address." >&2
		exit 1
	}
	range_to=$(resolve_pc "$4") || {
		echo "No symbol $4 in system.elf, and it is not a hex address." >&2
		exit 1
	}
	if [ "$range_from" = "$range_to" ]; then
		echo "START and END are the same address." >&2
		exit 1
	fi
fi
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
range)
	symbol=range
	done_pat="range returned"
	extra="--timer-interrupt --count range --range-from 0x$range_from --range-to 0x$range_to"
	# Same limit as timer: the no-disk boot can reach the panic spin.
	max_tick=100000000000
	;;
*)
	echo "Usage: labs/i386/gem5/launch.sh [paging|timer|schedule|copy_page]" >&2
	echo "       labs/i386/gem5/launch.sh from START to END" >&2
	exit 1
	;;
esac

if [ "$mode" = range ]; then
	addr=$range_from
	printf 'range from 0x%x to 0x%x\n' "0x$range_from" "0x$range_to"
elif [ "$mode" = copy_page ]; then
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
	# nm prints the address without a 0x prefix.
	printf '%s at 0x%x\n' "$symbol" "0x$addr"
fi

export GEM5_ROOT="$root/.gem5"
out=labs/i386/gem5/m5out
mkdir -p "$out"
log=$out/gem5.log
rm -f "$out/pipe.raw" "$out/instructions.txt" "$out/pipeline.txt" \
	"$out/pipeline.html"
if ! "$gem5" --outdir="$out" --debug-file=pipe.raw \
	labs/i386/gem5/linux-0.11-x86.py \
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

stats=$out/stats.txt
field() {
	val=$(awk -v name="$1" '$1 == name { print $2; exit }' "$stats")
	# gem5 omits a counter that stayed zero.
	if [ -z "$val" ]; then
		val=0
	fi
	printf '%s' "$val"
}
# Average miss latency is in ticks. The CPU clock period is the
# length of one cycle, also in ticks (1000 at 1 GHz).
cycles_from_ticks() {
	awk -v t="$1" -v c="$2" 'BEGIN {
		if (t == "" || t == "nan" || t+0 == 0 || c == "" || c == "nan" || c+0 == 0)
			print "0"
		else
			printf "%.2f", t / c
	}'
}

cycles=$(field system.cpu.numCycles)
insts=$(field system.cpu.commitStats0.numInsts)
ops=$(field system.cpu.commitStats0.numOps)
cpi=$(field system.cpu.cpi)
loads=$(field system.cpu.commitStats0.numLoadInsts)
stores=$(field system.cpu.commitStats0.numStoreInsts)
iacc=$(field system.cpu.icache.overallAccesses::total)
imiss=$(field system.cpu.icache.overallMisses::total)
irate=$(field system.cpu.icache.overallMissRate::total)
dacc=$(field system.cpu.dcache.overallAccesses::total)
dmiss=$(field system.cpu.dcache.overallMisses::total)
drate=$(field system.cpu.dcache.overallMissRate::total)
dlat=$(cycles_from_ticks \
	"$(field system.cpu.dcache.overallAvgMissLatency::total)" \
	"$(field system.cpu_clk_domain.clock)")
branches=$(field system.cpu.branchPred.committed_0::total)
bmiss=$(field system.cpu.branchPred.mispredicted_0::total)
sq=$(field system.cpu.rename.SQFullEvents)
iq=$(field system.cpu.rename.IQFullEvents)
serial=$(field "system.cpu.rename.status::SerializeStall")
if [ "$cycles" = 0 ] || [ "$insts" = 0 ]; then
	echo "stats.txt is missing cycles or instructions. See $stats" >&2
	exit 1
fi
# A miss rate left out of stats.txt is 0, same as a counter.
if [ "$irate" = "nan" ]; then irate=0; fi
if [ "$drate" = "nan" ]; then drate=0; fi

printf 'cycles              %12s   system.cpu.numCycles\n' "$cycles"
printf 'instructions        %12s   system.cpu.commitStats0.numInsts\n' "$insts"
printf 'microops            %12s   system.cpu.commitStats0.numOps\n' "$ops"
printf 'cpi                 %12s   system.cpu.cpi\n' "$cpi"
printf 'loads               %12s   system.cpu.commitStats0.numLoadInsts\n' "$loads"
printf 'stores              %12s   system.cpu.commitStats0.numStoreInsts\n' "$stores"
printf 'L1 I accesses       %12s   system.cpu.icache.overallAccesses::total\n' "$iacc"
printf 'L1 I miss rate      %12s   system.cpu.icache.overallMissRate::total\n' "$irate"
printf 'L1 I misses         %12s   system.cpu.icache.overallMisses::total\n' "$imiss"
printf 'L1 D accesses       %12s   system.cpu.dcache.overallAccesses::total\n' "$dacc"
printf 'L1 D miss rate      %12s   system.cpu.dcache.overallMissRate::total\n' "$drate"
printf 'L1 D misses         %12s   system.cpu.dcache.overallMisses::total\n' "$dmiss"
printf 'L1 D miss cycles    %12s   system.cpu.dcache.overallAvgMissLatency::total\n' "$dlat"
printf 'branches            %12s   system.cpu.branchPred.committed_0::total\n' "$branches"
printf 'branch mispredicts %12s   system.cpu.branchPred.mispredicted_0::total\n' "$bmiss"
printf 'store-queue stalls  %12s   system.cpu.rename.SQFullEvents\n' "$sq"
printf 'issue-queue stalls  %12s   system.cpu.rename.IQFullEvents\n' "$iq"
printf 'serialize stalls    %12s   system.cpu.rename.status::SerializeStall\n' "$serial"
echo "Model: gem5 O3 at 1 GHz, 32KiB L1 I and D. Full log: $log"
python3 labs/i386/gem5/window_view.py \
	--trace "$out/pipe.raw" \
	--mode "$mode" \
	--start "$addr" \
	--instructions "$out/instructions.txt" \
	--pipeline "$out/pipeline.txt" \
	--html "$out/pipeline.html" \
	--pipeview "$root/.gem5/util/o3-pipeview.py" \
	--pythonpath "$root/.gem5/src/python" \
	--expect "$ops"
rm -f "$out/pipe.raw"
echo "Instructions: $out/instructions.txt"
echo "Pipeline: $out/pipeline.txt"
echo "Pipeline view: $out/pipeline.html"
