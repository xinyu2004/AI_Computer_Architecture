# Filling the page tables

[中文](../../zh/cases/paging.md)

[Chapters](../cases.md)

## What this chapter is for

`setup_paging` writes a page directory and four page tables at physical address 0, 20KiB in all, identity-maps the first 16MB, and then sets [PG](../i386_instructions.md#pg) in [CR0](../i386_instructions.md#cr0). Those writes fill the store queue. The cycles come from gem5's out-of-order model at 1 GHz, with 32KiB L1 caches and 64-byte lines. The two frames that replace the page-table root are in the [mechanism pictures](../mech.md#mmu).

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
cycles                     36047   system.cpu.numCycles
instructions               12307   system.cpu.commitStats0.numInsts
microops                   65577   system.cpu.commitStats0.numOps
cpi                     2.928984   system.cpu.cpi
loads                          1   system.cpu.commitStats0.numLoadInsts
stores                      9220   system.cpu.commitStats0.numStoreInsts
L1 I accesses                 56   system.cpu.icache.overallAccesses::total
L1 I miss rate          0.214286   system.cpu.icache.overallMissRate::total
L1 I misses                   12   system.cpu.icache.overallMisses::total
L1 D accesses               9223   system.cpu.dcache.overallAccesses::total
L1 D miss rate          0.034913   system.cpu.dcache.overallMissRate::total
L1 D misses                  322   system.cpu.dcache.overallMisses::total
L1 D miss cycles           56.11   system.cpu.dcache.overallAvgMissLatency::total
branches                    9217   system.cpu.branchPred.committed_0::total
branch mispredicts           26   system.cpu.branchPred.mispredicted_0::total
store-queue stalls         23832   system.cpu.rename.SQFullEvents
issue-queue stalls             4   system.cpu.rename.IQFullEvents
serialize stalls             172   system.cpu.rename.status::SerializeStall
```

CPI is 2.93 on 9220 stores and 1 load. Of the 322 data misses, 320 are writes, one line for each 64 bytes of those 20KiB. The other two are reads. The miss rate is 0.035, and an average data miss takes 56.11 cycles. The store-queue stalls are the pipeline stall for this loop: `stosl` fills the queue faster than memory drains it. Issue-queue stalls are 4. For the run you just started, the numbers the script prints are the ones to use.

The same run writes `m5out/instructions.txt`, `m5out/pipeline.txt`, and `m5out/pipeline.html`. The fill is one loop repeated, so the files keep the first 32 microops and the last 12. On this run that was 44 of 65577 microops, and the last of them write CR3, CR0, and then return. 65577 is the same number as `microops`.
