# The timer interrupt

[中文](../../zh/cases/timer.md)

[Chapters](../cases.md)

## What this chapter is for

IRQ0 comes through the 8259 as vector `0x20` and enters `_timer_interrupt`. The handler saves registers, increments `jiffies`, writes the EOI, calls `do_timer`, and returns with `iret`. This gem5 run has no root disk, so task 1 calls `panic("Unable to mount root")` and spins. The first timer interrupt is taken from that spin. The interrupted code is in the kernel, so this tick does not call `_schedule`.

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
cycles                   866   system.cpu.numCycles
instructions              57   system.cpu.commitStats0.numInsts
microops                 211   system.cpu.commitStats0.numOps
L1 I misses                3   system.cpu.icache.overallMisses::total
L1 D misses               19   system.cpu.dcache.overallMisses::total
store-queue stalls         0   system.cpu.rename.SQFullEvents
serialize stalls         284   system.cpu.rename.status::SerializeStall
```

The return address is `0x7a06`, the `jmp` in `_panic`. The store queue does not fill. The pipeline stall is the serialize stall. The exit line is `timer_interrupt returned`. QEMU and Bochs are still where the kernel reaches a shell.
