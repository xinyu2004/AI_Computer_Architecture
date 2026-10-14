# Watch the cost of that execution in gem5

[中文](../zh/gem5.md)

This is learning step 4. The first path is `setup_paging` in `boot/head.s`, the same function the Bochs paging stop lands on. The run counts cycles, CPI, the load and store mix, L1 miss rates, the average data-cache miss latency, committed branches, and the rename stalls for that function. It also writes that window's microops and a pipeline diagram.

`src/linux-0.11/` is not modified. There is no gem5 opcode in the kernel. Pressing F5 still debugs the QEMU machine. `labs/i386/bochs/launch.sh` is unchanged.

## What this run is

gem5's usual x86 full-system loader refuses a 32-bit kernel and builds a 64-bit long-mode handoff. `labs/i386/gem5/linux-0.11-x86.patch` changes the local gem5 tree so an ELF32 kernel starts differently:

- Protected mode is already on. Paging is off. Physical-address extension and long mode stay off, so the page tables `setup_paging` writes are the 32-bit ones in `head.s`.
- Code uses selector 8 and data uses selector `0x10`, the flat segments in the kernel GDT at `_gdt`.
- The CPU starts at `setup_paging`. That symbol moves every time the kernel is linked. `labs/i386/gem5/launch.sh` prints the address from `tools/system.elf`.
- In the real boot, `ret` at the end of `setup_paging` enters `_main`, because `after_page_tables` pushed that return address. This run plants a different return address, so `ret` lands on an `hlt` at physical `0x80000`. The stack slot is at `0x7F000`. Both are above the loaded kernel and outside the five pages the function clears at physical 0.

The CPU is gem5's out-of-order model, clocked at 1 GHz, with a 32KiB L1 instruction cache and a 32KiB L1 data cache. Cache lines are 64 bytes. These cycles are from that model. They are not a cycle-accurate 80386. The instructions are still the 1991 function.

The run does not boot the floppy, does not log in, and does not start the scheduler. QEMU and Bochs remain the places where the whole kernel runs.

## How to check

From the repository root, after `script/bootstrap.sh`:

```bash
make -C src/linux-0.11
labs/i386/gem5/build.sh
labs/i386/gem5/launch.sh
labs/i386/gem5/launch.sh timer
labs/i386/gem5/launch.sh schedule
labs/i386/gem5/launch.sh copy_page
labs/i386/gem5/launch.sh from START to END
```

`labs/i386/gem5/build.sh` clones gem5 v25.1.0.1 into `.gem5/`, applies `labs/i386/gem5/linux-0.11-x86.patch`, and compiles `build/X86/gem5.opt`. `.gem5/` is build state and stays out of git. The first build takes a while. Later builds reuse it. The patch changes gem5 so this 32-bit kernel can start. It is not a kernel patch.

`labs/i386/gem5/launch.sh` is the command to run. It reads the symbol address from `tools/system.elf` and prints the counters. gem5 itself needs a description of the machine, which is `labs/i386/gem5/linux-0.11-x86.py`: an out-of-order x86 CPU, 32KiB L1 caches, 16MiB of memory, and a 1 GHz clock. Every measurement on this step uses that same machine. `launch.sh` passes the kernel and which path to count. The Python file is not run on its own.

`labs/i386/gem5/launch.sh` then prints this build's `setup_paging` address and the counters from `labs/i386/gem5/m5out/stats.txt`. One run of this tree printed:

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

The address changes when the kernel is relinked. The script prints the address and the counters for the run you just started. A counter that stayed at zero is omitted from `stats.txt`; the script prints that line as 0. Every line is the same window as `numCycles`. `instructions` counts x86 instructions. `microops` counts what the pipeline renames; `stosl` and `rep movsl` expand into many of them. `cpi` divides the cycles by those x86 instructions. `loads` and `stores` are the committed memory operations. The access and miss-rate lines give the miss counts a denominator. `L1 D miss cycles` is `overallAvgMissLatency` divided by `system.cpu_clk_domain.clock` (1000 ticks at 1 GHz), so it is the average data-cache miss in cycles. `branches` and `branch mispredicts` count committed branches. `issue-queue stalls` counts rename blocking because the issue queue was full. The rest of `labs/i386/gem5/m5out/stats.txt` stays on disk.

`setup_paging` writes five pages at physical 0: the page directory and four page tables, 20KiB. At 64-byte lines that is 320 lines. The data cache records 320 write misses and 2 read misses. The store-queue counter is how many times rename blocked because the store queue was full. That is the pipeline stall for this loop: `stosl` fills the queue faster than memory drains it. The serialize-stall line is printed for every run. On this path the store queue is the stall that dominates.

The full gem5 log is `labs/i386/gem5/m5out/gem5.log`. It includes a warning that the DRAM chip gem5 instantiated is larger than the 16MiB range. The L1 miss timing still comes from that DRAM model. The exit line is `setup_paging returned`, which is the `hlt` after `ret`, so the writes to CR3 and CR0 have already executed.

## The instructions and the pipeline

The same run writes `labs/i386/gem5/m5out/instructions.txt`, `labs/i386/gem5/m5out/pipeline.txt`, and `labs/i386/gem5/m5out/pipeline.html`. Recording starts when the counters reset and stops when the window ends, so the boot before the window is left out.

`instructions.txt` lists the committed microops in program order. Several microops share one x86 instruction, and the address is `pc.upc`. `timer` and `schedule` keep the whole window. `paging` keeps the first 32 microops and the last 12, so the writes to CR3 and CR0 stay in the file. `copy_page` keeps the first 32 microops of the `rep movsl` loop. On this run the notes in those files were "the first 32 and the last 12 of 65577 microops" and "the first 32 of 6151 microops". That count is the same number as the `microops` line. `launch.sh` stops if the two disagree.

`pipeline.txt` draws one row for each microop in that list. Time moves right. `f` is fetch, `d` decode, `n` rename, `p` dispatch, `i` issue, `c` complete, and `r` retire. A dot is a cycle in that stage or waiting for the next one. A row is 72 cycles wide and is aligned to that microop's own fetch, so the timer window, which starts about 10ms into the boot, still prints a short row. A microop longer than 72 cycles continues on the following lines. The run stops on a PC, the same window the counters use. Each letter is one pipeline stage, and the diagram is written after the run.

The timer file starts at `push %ds`:

```text
0x00006ee4.0   PUSH_S : rdsel   t1d, DS
0x00006ee4.1   PUSH_S : st   t1d, SS:[t0d + esp + 0xfffffffffffffffc]
0x00006ee4.2   PUSH_S : subi   esp, esp, 0x4
```

The three microops are fetched together:

```text
[....................fdn.ic.r............................................]-(    10070352000) 0x00006ee4.0 PUSH_S                    [  28942374]
[....................fdn.pic.r...........................................]-(    10070352000) 0x00006ee4.1 PUSH_S                    [  28942375]
[....................fdn.ic..r...........................................]-(    10070352000) 0x00006ee4.2 PUSH_S                    [  28942376]
```

`labs/i386/gem5/m5out/pipeline.html` is the same list, one microop per row, opened in a browser. The left edge of every bar is fetch. Each color is one stage: fetch, decode, rename, dispatch, issue, complete, retire. The width is cycles in that stage or waiting for the next one, and every row uses the same scale, so a miss is a longer bar. The number at the right is that microop's cycle count. A long loop uses the same slice as the text file.

The text file continues through `iret`. A later row is the microop that writes DS, `wrdl DS` in the instruction file. Dispatch is done, and issue waits:

```text
[................................fdn.p...............................ic.r]-(    10070352000) 0x00006ef1.9 MOV_S_R                   [  28942444]
```

## The timer interrupt

`labs/i386/gem5/launch.sh timer` starts at `startup_32`. The same patch leaves the CPU in 32-bit protected mode with paging off, and programs the 8259 the way `boot/setup.s` does: master vector base `0x20`, slave base `0x28`, both masks `0xFF`. IRQ0 stays on the 8259 and is delivered to the CPU. Boot parameters at `0x90000` describe 16MiB of memory, and `0x901FC` holds root device `0x0301`. The stack pointer is the value stored at `_stack_start`.

The kernel then executes its own `startup_32`, including `setup_paging`, and continues into `_main`. This run has no root disk, so task 1's `setup()` calls `panic("Unable to mount root")` and spins on the `jmp` at `_panic+0x22`. About 10ms after `sched_init` programs the PIT, IRQ0 is taken from that spin. The handler is the 1991 `_timer_interrupt`: it saves registers, increments `_jiffies`, writes the EOI, calls `_do_timer`, and returns with `iret`. The interrupted code is in the kernel, so this tick does not call `_schedule`.

Counters restart when the PC becomes `_timer_interrupt`, before `push %ds`. They stop when the PC becomes the interrupted instruction, before that instruction runs. On this run that instruction is the `jmp` at `0x7a06`. One run printed:

```text
_timer_interrupt at 0x6ee4
cycles                       866   system.cpu.numCycles
instructions                  57   system.cpu.commitStats0.numInsts
microops                     211   system.cpu.commitStats0.numOps
cpi                    15.192982   system.cpu.cpi
loads                         32   system.cpu.commitStats0.numLoadInsts
stores                        14   system.cpu.commitStats0.numStoreInsts
L1 I accesses                 67   system.cpu.icache.overallAccesses::total
L1 I miss rate          0.044776   system.cpu.icache.overallMissRate::total
L1 I misses                    3   system.cpu.icache.overallMisses::total
L1 D accesses                112   system.cpu.dcache.overallAccesses::total
L1 D miss rate          0.169643   system.cpu.dcache.overallMissRate::total
L1 D misses                   19   system.cpu.dcache.overallMisses::total
L1 D miss cycles           51.68   system.cpu.dcache.overallAvgMissLatency::total
branches                      32   system.cpu.branchPred.committed_0::total
branch mispredicts           16   system.cpu.branchPred.mispredicted_0::total
store-queue stalls             0   system.cpu.rename.SQFullEvents
issue-queue stalls             6   system.cpu.rename.IQFullEvents
serialize stalls             284   system.cpu.rename.status::SerializeStall
```

The store queue does not fill. The pipeline stall on this path is the serialize stall: rename waits while a serializing instruction drains the pipeline. CPI is 15.2. The handler commits 32 loads and 14 stores, and 16 of 32 branches were mispredicted. Issue-queue stalls are 6. The CPU model is the same one used for `setup_paging`. The exit line in `labs/i386/gem5/m5out/gem5.log` is `timer_interrupt returned`.

QEMU and Bochs still boot the disks and reach a shell. This gem5 command does not.

## schedule

`labs/i386/gem5/launch.sh schedule` uses that same boot and counts the first `_schedule`. Task 0 reaches it from `pause()` after `fork` returns. The call selects task 1 and executes `ljmp` to TSS selector `0x30`.

Counters restart when the PC becomes `_schedule`, before `push %esi`. They stop when the PC becomes task 1's first instruction, before that instruction runs. On this run that address is linear `0x4012f0c`: CS is `0x0f`, the segment base is `0x4000000`, and the offset `0x12f0c` is the return from `fork` in `_main`. The task switch is inside the window. The `ret` at the end of `_schedule` runs when task 0 is switched back. This run does not get there. Task 1 continues and later hits the same `panic("Unable to mount root")`.

One run printed:

```text
_schedule at 0x6814
cycles                      1205   system.cpu.numCycles
instructions                 730   system.cpu.commitStats0.numInsts
microops                    1525   system.cpu.commitStats0.numOps
cpi                     1.650685   system.cpu.cpi
loads                        162   system.cpu.commitStats0.numLoadInsts
stores                        20   system.cpu.commitStats0.numStoreInsts
L1 I accesses                168   system.cpu.icache.overallAccesses::total
L1 I miss rate          0.047619   system.cpu.icache.overallMissRate::total
L1 I misses                    8   system.cpu.icache.overallMisses::total
L1 D accesses                216   system.cpu.dcache.overallAccesses::total
L1 D miss rate          0.004630   system.cpu.dcache.overallMissRate::total
L1 D misses                    1   system.cpu.dcache.overallMisses::total
L1 D miss cycles           53.00   system.cpu.dcache.overallAvgMissLatency::total
branches                     281   system.cpu.branchPred.committed_0::total
branch mispredicts           27   system.cpu.branchPred.mispredicted_0::total
store-queue stalls             0   system.cpu.rename.SQFullEvents
issue-queue stalls           138   system.cpu.rename.IQFullEvents
serialize stalls             274   system.cpu.rename.status::SerializeStall
```

The store queue does not fill. CPI is 1.65. The window commits 162 loads and 20 stores, and 27 of 281 branches were mispredicted. Issue-queue stalls are 138, beside 274 serialize stalls around the task switch. The exit line is `schedule switched`. A second run printed the same counters.

## copy_page

`copy_page` in `mm/memory.c` is a macro, so it has no symbol. The body is `rep movsl` of 1024 dwords, 4KiB. `-O2` inlines it in `_un_wp_page`. `labs/i386/gem5/launch.sh copy_page` finds that `rep movsl` and counts the first time it runs. On this link the instruction is at `0x93a3`. A relink moves it; the script prints the address it found.

That first copy is the write-protect copy in `un_wp_page`, after task 1 has started and written a shared page. Counters restart when the PC becomes the `rep movsl`, before it runs, and stop when the PC becomes the next instruction, before that instruction runs.

gem5's instruction counter stays at 1 for the whole repetition: the `rep movsl` itself. The work shows up as microops. One run printed:

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

CPI is 2565 because it divides by the x86 instruction count, which stays at 1. The 6151 microops are the size of the work. The window commits 1024 loads and 1024 stores. The data miss rate is 0.47, and an average data miss takes 51.44 cycles. The data cache recorded 1030 reads and 1013 writes. The 970 read misses include 61 that brought in a new line and 909 that hit a line already being filled. `get_free_page` had just cleared the destination page, so the stores hit. The store-queue stalls are the pipeline stall for this loop, the same kind `setup_paging` hits with `stosl`. Issue-queue stalls are 147. The exit line is `copy_page returned`. A second run printed the same counters.

This copy happens on the way to the missing-root panic. QEMU and Bochs still boot the disks and reach a shell.

## A window you name

```bash
labs/i386/gem5/launch.sh from START to END
```

`START` and `END` are symbols in `tools/system.elf`, or hex addresses such as `0x6ee4`. A symbol is read from the kernel you just linked. A hex address is used as written. A name that is both a symbol and a hex spelling is the symbol.

The boot is the same one as `timer`: `startup_32`, no root disk, the same out-of-order CPU. Counters reset when the PC becomes `START`, before that instruction runs. They stop when the PC becomes `END`, before that instruction runs. `END` is the first instruction outside the window. The files keep every committed microop in the window, and `launch.sh` stops if that count disagrees with `microops`.

- Both addresses are fixed before the run. An end that is computed while the kernel runs stays a named argument. Task 1's first instruction after the `ljmp` in `_schedule` is `launch.sh schedule`.
- `START` has to execute on this boot. Code that runs only after a root disk is mounted is not reached. The run then stops at the tick limit, and `launch.sh` fails.
- `END` is the address of an instruction that runs after `START`. The middle of an instruction does not match. If that PC occurs only before `START`, the window does not close.
- A long stretch is kept whole, so `setup_paging` and the `rep movsl` stay the named arguments, which slice those loops.
- The four named arguments are unchanged.

Login, a BIOS boot, and a gem5 session inside F5 are outside this step.
