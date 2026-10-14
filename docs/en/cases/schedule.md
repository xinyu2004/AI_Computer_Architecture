# The task switch

[中文](../../zh/cases/schedule.md)

[Chapters](../cases.md)

## What this chapter is for

`schedule` picks the next task. The switch itself is the `ljmp` in `switch_to`, aimed at that task's TSS. Task 1's code segment base is `0x4000000`. The linear address gem5 reports is that base plus the offset inside the segment.

## What to read

`schedule` at line 104 of `src/linux-0.11/kernel/sched.c`. The `ljmp` is in the `switch_to` macro at line 171 of `include/linux/sched.h`. After `fork` returns, task 0 calls `pause()` in `init/main.c`, and that call enters `_schedule` for the first time.

## How to start

Bochs:

```bash
make -C src/linux-0.11
labs/i386/bochs/launch.sh
```

The script prints the address of `_schedule`. On this link the example is `0x6814`. Use the address the script printed:

```text
pb 0x6814
c
info tss
sreg
```

Look at `sreg` again after the `ljmp`.

gem5:

```bash
labs/i386/gem5/launch.sh schedule
```

The counters start at the entry of `_schedule`, before `push %esi`, and stop before task 1's first instruction runs. The `ret` at the end of `_schedule` runs when task 0 is switched back. This run does not get there.

## What you should see

Bochs `info tss` prints the task state segment. After the far jump, TR in `sreg` points at the current task's TSS.

One gem5 run of this tree printed:

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

The switch in the log uses TSS selector `0x30`, CS `0x0f`, and linear address `0x4012f0c`. `0x4000000` is task 1's segment base. `0x12f0c` is the offset of the return from `fork` in `_main`. The pipeline stall is the serialize stall. The exit line is `schedule switched`. The run still goes on to `panic("Unable to mount root")`.
