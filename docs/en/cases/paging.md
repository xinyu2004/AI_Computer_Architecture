# Filling the page tables

[中文](../../zh/cases/paging.md)

[Chapters](../cases.md)

## What this chapter is for

`setup_paging` writes a page directory and four page tables at physical address 0, 20KiB in all, identity-maps the first 16MB, and then sets CR0.PG. Those writes fill the store queue. The cycles come from gem5's out-of-order model at 1 GHz, with 32KiB L1 caches and 64-byte lines.

## What to read

`setup_paging` at line 199 of `src/linux-0.11/boot/head.s`. It clears five pages with `stosl`, fills the four page tables, writes CR3 and CR0, and then `ret`.

## Predict first

20KiB at 64 bytes a line is 320 lines. The data-cache write misses should land near 320.

## How to start

From the repository root, after `labs/i386/gem5/build.sh` has compiled gem5:

```bash
make -C src/linux-0.11
labs/i386/gem5/launch.sh
```

The script reads the address of `setup_paging` from `tools/system.elf` and prints it. On this link the example is `0x5450`. The counters start at the function entry and stop on the `hlt` the `ret` lands on. The machine is `labs/i386/gem5/linux-0.11-x86.py`. Details are in [gem5](../gem5.md).

To watch PG change from clear to set, use Bochs. Do not single-step the fill loop. The script prints the same address:

```text
pb 0x5450
c
```

On the first instruction, `movl $1024*5,%ecx`, PG is still clear. Step until `movl %eax,%cr0` has executed, then run `creg`.

## What you should see

In Bochs, PG is set and CR3 is 0. `page 0` translates linear address 0.

One gem5 run of this tree printed:

```text
setup_paging at 0x5450
cycles                 36047   system.cpu.numCycles
instructions           12307   system.cpu.commitStats0.numInsts
microops               65577   system.cpu.commitStats0.numOps
L1 I misses               12   system.cpu.icache.overallMisses::total
L1 D misses              322   system.cpu.dcache.overallMisses::total
store-queue stalls     23832   system.cpu.rename.SQFullEvents
serialize stalls         172   system.cpu.rename.status::SerializeStall
```

Of the 322 data misses, 320 are writes, one line for each 64 bytes of those 20KiB. The other two are reads. The store-queue stalls are the pipeline stall for this loop: `stosl` fills the queue faster than memory drains it. For the run you just started, the numbers the script prints are the ones to use.
