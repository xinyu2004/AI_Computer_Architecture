# The timer interrupt

[中文](../../zh/cases/timer.md)

[Chapters](../cases.md)

## What this chapter is for

IRQ0 comes through the 8259 as vector `0x20` and enters `_timer_interrupt`. The handler saves registers, increments `jiffies`, writes the EOI, calls `do_timer`, and returns with [`iret`](../i386_instructions.md#iret). This gem5 run has no root disk, so task 1 calls `panic("Unable to mount root")` and spins. The first timer interrupt is taken from that spin. The interrupted code is in the kernel, so this tick does not call `_schedule`. The four frames of the saved state are in the [mechanism pictures](../mech.md#stack).

## What to read

`_timer_interrupt` at line 177 of `src/linux-0.11/kernel/system_call.s`. `boot/setup.s` sets the first 8259's vector base to `0x20`. `sched_init` in `kernel/sched.c` points the gate at the handler and unmasks IRQ0.

## How to start

Bochs, from the repository root:

```bash
make -C src/linux-0.11
labs/i386/bochs/launch.sh
```

The script prints the address of `_timer_interrupt`. On this link the example is `0x6ee4`. Use the address the script printed:

```text
pb 0x6ee4
c
info idt 0x20
sreg
```

gem5:

```bash
labs/i386/gem5/launch.sh timer
```

The counters start when the PC becomes `_timer_interrupt`, before `push %ds`, and stop before the interrupted instruction runs again. Details are in [gem5](../gem5.md).

## What you should see

`info idt 0x20` is a 32-bit interrupt gate, DPL 0, whose target is this handler.

One gem5 run of this tree printed:

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

The return address is `0x7a06`, the `jmp` in `_panic`. CPI is 15.2. The handler commits 32 loads and 14 stores, and 16 of 32 branches were mispredicted. The store queue does not fill. The pipeline stall is the serialize stall. Issue-queue stalls are 6. The exit line is `timer_interrupt returned`. QEMU and Bochs are still where the kernel reaches a shell.

The same run writes `m5out/instructions.txt`, `m5out/pipeline.txt`, and `m5out/pipeline.html` for this whole handler, from `push %ds` through `iret`. The instruction file starts:

```text
0x00006ee4.0   PUSH_S : rdsel   t1d, DS
0x00006ee4.1   PUSH_S : st   t1d, SS:[t0d + esp + 0xfffffffffffffffc]
0x00006ee4.2   PUSH_S : subi   esp, esp, 0x4
```

How to read the pipeline rows is in [gem5](../gem5.md).
