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

The switch in the log uses TSS selector `0x30`, CS `0x0f`, and linear address `0x4012f0c`. `0x4000000` is task 1's segment base. `0x12f0c` is the offset of the return from `fork` in `_main`. CPI is 1.65. Issue-queue stalls are 138, beside 274 serialize stalls. The exit line is `schedule switched`. The run still goes on to `panic("Unable to mount root")`.

The same run writes `m5out/instructions.txt`, `m5out/pipeline.txt`, and `m5out/pipeline.html` for this whole window, from `push %esi` through the task-switch microcode.
