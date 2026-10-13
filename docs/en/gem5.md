# Watch the cost of that execution in gem5

[中文](../zh/gem5.md)

This is learning step 4. The first path is `setup_paging` in `boot/head.s`, the same function the Bochs paging stop lands on. The run counts cycles, L1 cache misses, and one pipeline stall for that function.

`src/linux-0.11/` is not modified. There is no gem5 opcode in the kernel. Pressing F5 still debugs the QEMU machine. `labs/bochs/launch.sh` is unchanged.

## What this run is

gem5's usual x86 full-system loader refuses a 32-bit kernel and builds a 64-bit long-mode handoff. `labs/gem5/linux-0.11-x86.patch` changes the local gem5 tree so an ELF32 kernel starts differently:

- Protected mode is already on. Paging is off. Physical-address extension and long mode stay off, so the page tables `setup_paging` writes are the 32-bit ones in `head.s`.
- Code uses selector 8 and data uses selector `0x10`, the flat segments in the kernel GDT at `_gdt`.
- The CPU starts at `setup_paging`. That symbol moves every time the kernel is linked. `labs/gem5/launch.sh` prints the address from `tools/system.elf`.
- In the real boot, `ret` at the end of `setup_paging` enters `_main`, because `after_page_tables` pushed that return address. This run plants a different return address, so `ret` lands on an `hlt` at physical `0x80000`. The stack slot is at `0x7F000`. Both are above the loaded kernel and outside the five pages the function clears at physical 0.

The CPU is gem5's out-of-order model, clocked at 1 GHz, with a 32KiB L1 instruction cache and a 32KiB L1 data cache. Cache lines are 64 bytes. These cycles are from that model. They are not a cycle-accurate 80386. The instructions are still the 1991 function.

The run does not boot the floppy, does not log in, and does not start the scheduler. QEMU and Bochs remain the places where the whole kernel runs.

## How to check

From the repository root, after `script/bootstrap.sh`:

```bash
make -C src/linux-0.11
labs/gem5/build.sh
labs/gem5/launch.sh
labs/gem5/launch.sh timer
labs/gem5/launch.sh schedule
labs/gem5/launch.sh copy_page
```

`labs/gem5/build.sh` clones gem5 v25.1.0.1 into `.gem5/`, applies `labs/gem5/linux-0.11-x86.patch`, and compiles `build/X86/gem5.opt`. `.gem5/` is build state and stays out of git. The first build takes a while. Later builds reuse it. The patch changes gem5 so this 32-bit kernel can start. It is not a kernel patch.

`labs/gem5/launch.sh` is the command to run. It reads the symbol address from `tools/system.elf` and prints the counters. gem5 itself needs a description of the machine, which is `labs/gem5/linux-0.11-x86.py`: an out-of-order x86 CPU, 32KiB L1 caches, 16MiB of memory, and a 1 GHz clock. Every measurement on this step uses that same machine. `launch.sh` passes the kernel and which path to count. The Python file is not run on its own.

`labs/gem5/launch.sh` then prints this build's `setup_paging` address and the counters from `m5out/stats.txt`. One run of this tree printed:

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

The address changes when the kernel is relinked. The script prints the address and the counters for the run you just started. A counter that stayed at zero is omitted from `stats.txt`; the script prints that line as 0. `instructions` counts x86 instructions. `microops` counts what the pipeline renames; `stosl` and `rep movsl` expand into many of them.

`setup_paging` writes five pages at physical 0: the page directory and four page tables, 20KiB. At 64-byte lines that is 320 lines. The data cache records 320 write misses and 2 read misses. The store-queue counter is how many times rename blocked because the store queue was full. That is the pipeline stall for this loop: `stosl` fills the queue faster than memory drains it. The serialize-stall line is printed for every run. On this path the store queue is the stall that dominates.

The full gem5 log is `m5out/gem5.log`. It includes a warning that the DRAM chip gem5 instantiated is larger than the 16MiB range. The L1 miss timing still comes from that DRAM model. The exit line is `setup_paging returned`, which is the `hlt` after `ret`, so the writes to CR3 and CR0 have already executed.

## The timer interrupt

`labs/gem5/launch.sh timer` starts at `startup_32`. The same patch leaves the CPU in 32-bit protected mode with paging off, and programs the 8259 the way `boot/setup.s` does: master vector base `0x20`, slave base `0x28`, both masks `0xFF`. IRQ0 stays on the 8259 and is delivered to the CPU. Boot parameters at `0x90000` describe 16MiB of memory, and `0x901FC` holds root device `0x0301`. The stack pointer is the value stored at `_stack_start`.

The kernel then executes its own `startup_32`, including `setup_paging`, and continues into `_main`. This run has no root disk, so task 1's `setup()` calls `panic("Unable to mount root")` and spins on the `jmp` at `_panic+0x22`. About 10ms after `sched_init` programs the PIT, IRQ0 is taken from that spin. The handler is the 1991 `_timer_interrupt`: it saves registers, increments `_jiffies`, writes the EOI, calls `_do_timer`, and returns with `iret`. The interrupted code is in the kernel, so this tick does not call `_schedule`.

Counters restart when the PC becomes `_timer_interrupt`, before `push %ds`. They stop when the PC becomes the interrupted instruction, before that instruction runs. On this run that instruction is the `jmp` at `0x7a06`. One run printed:

```text
_timer_interrupt at 0x6ee4
cycles                   866   system.cpu.numCycles
instructions              57   system.cpu.commitStats0.numInsts
microops                 211   system.cpu.commitStats0.numOps
L1 I misses                3   system.cpu.icache.overallMisses::total
L1 D misses               19   system.cpu.dcache.overallMisses::total
store-queue stalls         0   system.cpu.rename.SQFullEvents
serialize stalls         284   system.cpu.rename.status::SerializeStall
```

The store queue does not fill. The pipeline stall on this path is the serialize stall: rename waits while a serializing instruction drains the pipeline. The CPU model is the same one used for `setup_paging`. The exit line in `m5out/gem5.log` is `timer_interrupt returned`.

QEMU and Bochs still boot the disks and reach a shell. This gem5 command does not.

## schedule

`labs/gem5/launch.sh schedule` uses that same boot and counts the first `_schedule`. Task 0 reaches it from `pause()` after `fork` returns. The call selects task 1 and executes `ljmp` to TSS selector `0x30`.

Counters restart when the PC becomes `_schedule`, before `push %esi`. They stop when the PC becomes task 1's first instruction, before that instruction runs. On this run that address is linear `0x4012f0c`: CS is `0x0f`, the segment base is `0x4000000`, and the offset `0x12f0c` is the return from `fork` in `_main`. The task switch is inside the window. The `ret` at the end of `_schedule` runs when task 0 is switched back. This run does not get there. Task 1 continues and later hits the same `panic("Unable to mount root")`.

One run printed:

```text
_schedule at 0x6814
cycles                  1205   system.cpu.numCycles
instructions             730   system.cpu.commitStats0.numInsts
microops                1525   system.cpu.commitStats0.numOps
L1 I misses                8   system.cpu.icache.overallMisses::total
L1 D misses                1   system.cpu.dcache.overallMisses::total
store-queue stalls         0   system.cpu.rename.SQFullEvents
serialize stalls         274   system.cpu.rename.status::SerializeStall
```

The store queue does not fill. The pipeline stall is the serialize stall around the task switch. The exit line is `schedule switched`. A second run printed the same counters.

## copy_page

`copy_page` in `mm/memory.c` is a macro, so it has no symbol. The body is `rep movsl` of 1024 dwords, 4KiB. `-O2` inlines it in `_un_wp_page`. `labs/gem5/launch.sh copy_page` finds that `rep movsl` and counts the first time it runs. On this link the instruction is at `0x93a3`. A relink moves it; the script prints the address it found.

That first copy is the write-protect copy in `un_wp_page`, after task 1 has started and written a shared page. Counters restart when the PC becomes the `rep movsl`, before it runs, and stop when the PC becomes the next instruction, before that instruction runs.

gem5's instruction counter stays at 2 for the whole repetition. The work shows up as microops. One run printed:

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

The data cache recorded 1030 reads and 1013 writes. The 970 read misses include 61 that brought in a new line and 909 that hit a line already being filled. `get_free_page` had just cleared the destination page, so the stores hit. The store-queue stalls are the pipeline stall for this loop, the same kind `setup_paging` hits with `stosl`. The exit line is `copy_page returned`. A second run printed the same counters.

This copy happens on the way to the missing-root panic. QEMU and Bochs still boot the disks and reach a shell.

Login, a BIOS boot, and a gem5 session inside F5 are outside this step.
