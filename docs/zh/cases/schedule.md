# 任务切换

[English](../../en/cases/schedule.md)

[目录](../cases.md)

## 这一章要看懂什么

`schedule` 选出下一个任务。真正换任务的是 `switch_to` 里的 `ljmp`，目标是那个任务的 TSS。任务 1 的代码段基址是 `0x4000000`。gem5 看到的线性地址是这段基址加上段内偏移。

## 读哪里

`src/linux-0.11/kernel/sched.c` 第 104 行的 `schedule`。`include/linux/sched.h` 第 171 行的 `switch_to` 宏里有 `ljmp`。`fork` 返回后，任务 0 在 `init/main.c` 里调用 `pause()`，从而进入第一次 `_schedule`。

## 怎么启动

Bochs：

```bash
make -C src/linux-0.11
labs/i386/bochs/launch.sh
```

脚本打印 `_schedule` 的地址。这一次链接的例子是 `0x6814`。用脚本打出来的地址：

```text
pb 0x6814
c
info tss
sreg
```

`ljmp` 执行之后再看一次 `sreg`。

gem5：

```bash
labs/i386/gem5/launch.sh schedule
```

计数从 `_schedule` 入口、`push %esi` 之前开始，停在任务 1 的第一条指令还没执行的时候。`_schedule` 末尾的 `ret` 要等任务 0 被切回来。这一次到不了那里。

## 该看到什么

Bochs 的 `info tss` 打印任务状态段。远跳转之后，`sreg` 里的 TR 指向当前任务的 TSS。

gem5 这一次打印过：

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

日志里的切换是 TSS 选择子 `0x30`，CS `0x0f`，线性地址 `0x4012f0c`。其中 `0x4000000` 是任务 1 的段基址，`0x12f0c` 是 `_main` 里 `fork` 的返回偏移。CPI 是 1.65。issue queue 停顿是 138，旁边是 274 次 serialize stall。结束行是 `schedule switched`。这次运行随后仍会走到 `panic("Unable to mount root")`。

同一次运行把整个窗口写进 `m5out/instructions.txt`、`m5out/pipeline.txt` 和 `m5out/pipeline.html`，从 `push %esi` 一直到任务切换的微码。
