# 时钟中断

[English](../../en/cases/timer.md)

[目录](../cases.md)

## 这一章要看懂什么

IRQ0 经过 8259，以向量 `0x20` 进入 `_timer_interrupt`。处理函数保存寄存器，递增 `jiffies`，写 EOI，调用 `do_timer`，再用 `iret` 返回。这一次 gem5 运行没有根磁盘，任务 1 会 `panic("Unable to mount root")` 并自旋。第一次时钟中断从那段自旋里进来，被打断的代码在内核里，所以这一拍不调用 `_schedule`。

## 读哪里

`src/linux-0.11/kernel/system_call.s` 第 177 行的 `_timer_interrupt`。`boot/setup.s` 把第一片 8259 的向量基址设成 `0x20`。`kernel/sched.c` 的 `sched_init` 把这个门指到处理函数，并打开 IRQ0。

## 怎么启动

Bochs，在仓库根目录：

```bash
make -C src/linux-0.11
labs/i386/bochs/launch.sh
```

脚本打印 `_timer_interrupt` 的地址。这一次链接的例子是 `0x6ee4`。用脚本打出来的地址：

```text
pb 0x6ee4
c
info idt 0x20
sreg
```

gem5：

```bash
labs/i386/gem5/launch.sh timer
```

计数从 PC 变成 `_timer_interrupt`、`push %ds` 还没执行时开始，停在被打断的那条指令还没再执行的时候。细节见 [gem5](../gem5.md)。

## 该看到什么

`info idt 0x20` 是一个 32 位中断门，DPL 为 0，目标是这个处理函数。

gem5 这一次打印过：

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

返回地址是 `0x7a06`，即 `_panic` 里的 `jmp`。CPI 是 15.2。处理函数提交了 32 次 load 和 14 次 store，32 次分支里有 16 次预测错。store queue 没有被填满。流水线停顿是 serialize stall。issue queue 停顿是 6。结束行是 `timer_interrupt returned`。能进 shell 的仍是 QEMU 和 Bochs。

同一次运行把整个处理函数写进 `m5out/instructions.txt`、`m5out/pipeline.txt` 和 `m5out/pipeline.html`，从 `push %ds` 一直到 `iret`。指令文件的开头是：

```text
0x00006ee4.0   PUSH_S : rdsel   t1d, DS
0x00006ee4.1   PUSH_S : st   t1d, SS:[t0d + esp + 0xfffffffffffffffc]
0x00006ee4.2   PUSH_S : subi   esp, esp, 0x4
```

流水线每一行怎么读，见 [gem5](../gem5.md)。
