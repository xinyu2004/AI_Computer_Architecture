# Copying a page

[中文](../../zh/cases/copy-page.md)

[Chapters](../cases.md)

## What this chapter is for

When a shared page is written, the kernel allocates another page and copies 4KiB. `copy_page` is a macro. Its body is a `rep movsl` of 1024 dwords. Like the `stosl` in `setup_paging`, it fills the store queue. One loop fills page tables. The other copies a page.

## What to read

`copy_page` at line 54 of `src/linux-0.11/mm/memory.c`. `-O2` inlines it into `un_wp_page` at line 225. There is no `copy_page` symbol. `labs/gem5/launch.sh copy_page` finds the `rep movsl` inside `_un_wp_page` on every run.

## How to start

From the repository root:

```bash
make -C src/linux-0.11
labs/gem5/launch.sh copy_page
```

The script prints the address it found. On this link the example is `0x93a3`. The counters start before that `rep movsl` runs and stop before the next instruction runs. The first execution is the copy task 1 makes when it writes a shared page, before `panic("Unable to mount root")`.

The `stosl` in [Filling the page tables](paging.md) is the loop to put beside this one.

## What you should see

gem5 counts the whole repetition as 2 instructions. The work shows up as microops. One run of this tree printed:

```text
copy_page at 0x93a3
cycles                  2565   system.cpu.numCycles
instructions               2   system.cpu.commitStats0.numInsts
microops                6155   system.cpu.commitStats0.numOps
L1 I misses                0   system.cpu.icache.overallMisses::total
L1 D misses              970   system.cpu.dcache.overallMisses::total
store-queue stalls       917   system.cpu.rename.SQFullEvents
serialize stalls          20   system.cpu.rename.status::SerializeStall
```

The data cache recorded 1030 reads and 1013 writes. Of the 970 read misses, 61 brought in a new line and 909 hit a line already being filled. `get_free_page` had just cleared the destination page, so the stores hit. The store-queue stalls are the pipeline stall for this loop. The exit line is `copy_page returned`.
