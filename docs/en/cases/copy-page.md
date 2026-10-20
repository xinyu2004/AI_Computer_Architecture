# Copying a page

[中文](../../zh/cases/copy-page.md)

[Chapters](../cases.md)

## What this chapter is for

When a shared page is written, the kernel allocates another page and copies 4KiB. `copy_page` is a macro. Its body is a `rep movsl` of 1024 dwords. Like the `stosl` in `setup_paging`, it fills the store queue. One loop fills page tables. The other copies a page. The source, the destination, and the count are on the [copy picture](../mech.md#copy).

## What to read

`copy_page` at line 54 of `src/linux-0.11/mm/memory.c`. `-O2` inlines it into `un_wp_page` at line 225. There is no `copy_page` symbol. `labs/i386/gem5/launch.sh copy_page` finds the `rep movsl` inside `_un_wp_page` on every run.

## How to start

From the repository root:

```bash
make -C src/linux-0.11
labs/i386/gem5/launch.sh copy_page
```

The script prints the address it found. On this link the example is `0x93a3`. The counters start before that `rep movsl` runs and stop before the next instruction runs. The first execution is the copy task 1 makes when it writes a shared page, before `panic("Unable to mount root")`.

The `stosl` in [Filling the page tables](paging.md) is the loop to put beside this one.

## What you should see

gem5 counts the whole repetition as 1 instruction, the `rep movsl` itself. The work shows up as microops. One run of this tree printed:

```text
copy_page at 0x93a3
cycles                      2565   system.cpu.numCycles
instructions                   1   system.cpu.commitStats0.numInsts
microops                    6151   system.cpu.commitStats0.numOps
cpi                  2565.000000   system.cpu.cpi
loads                       1024   system.cpu.commitStats0.numLoadInsts
stores                      1024   system.cpu.commitStats0.numStoreInsts
L1 I accesses                  2   system.cpu.icache.overallAccesses::total
L1 I miss rate                 0   system.cpu.icache.overallMissRate::total
L1 I misses                    0   system.cpu.icache.overallMisses::total
L1 D accesses               2043   system.cpu.dcache.overallAccesses::total
L1 D miss rate          0.474792   system.cpu.dcache.overallMissRate::total
L1 D misses                  970   system.cpu.dcache.overallMisses::total
L1 D miss cycles           51.44   system.cpu.dcache.overallAvgMissLatency::total
branches                    1024   system.cpu.branchPred.committed_0::total
branch mispredicts            1   system.cpu.branchPred.mispredicted_0::total
store-queue stalls           917   system.cpu.rename.SQFullEvents
issue-queue stalls           147   system.cpu.rename.IQFullEvents
serialize stalls              20   system.cpu.rename.status::SerializeStall
```

CPI is 2565 because it divides by the x86 instruction count, which stays at 1. The 6151 microops are the size of the work. The data cache recorded 1030 reads and 1013 writes. Of the 970 read misses, 61 brought in a new line and 909 hit a line already being filled. The miss rate is 0.47, and an average data miss takes 51.44 cycles. `get_free_page` had just cleared the destination page, so the stores hit. The store-queue stalls are the pipeline stall for this loop. Issue-queue stalls are 147. The exit line is `copy_page returned`.

The same run writes `m5out/instructions.txt`, `m5out/pipeline.txt`, and `m5out/pipeline.html`. The repetition is long, so the files keep the first 32 microops. On this run the note was "the first 32 of 6151 microops", and those lines are the start of the `rep movsl`. The 6151 is the same number as `microops`.
