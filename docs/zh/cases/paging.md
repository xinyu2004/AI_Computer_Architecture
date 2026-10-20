# 填页表

[English](../../en/cases/paging.md)

[目录](../cases.md)

## 这一章要看懂什么

`setup_paging` 在物理地址 0 写下页目录和四张页表，共 20KiB，把前 16MB 做成恒等映射，然后置上 [CR0](../i386_instructions.md#cr0) 的 [PG](../i386_instructions.md#pg)。这些写会把 store queue 填满。周期来自 gem5 的乱序模型，时钟 1 GHz，L1 各 32KiB，行长 64 字节。换根寄存器的两帧在 [机制图](../mech.md#mmu)。

## 读哪里

`src/linux-0.11/boot/head.s` 第 199 行的 `setup_paging`。它先用 `stosl` 清零五页，再填四张页表，然后写 CR3 和 CR0，最后 `ret`。

## 先猜

20KiB、64 字节一行，是 320 行。数据 cache 的写 miss 应落在 320 这一档。

## 怎么启动

在仓库根目录，gem5 已经由 `labs/i386/gem5/build.sh` 编好之后：

```bash
make -C src/linux-0.11
labs/i386/gem5/launch.sh
```

脚本从 `tools/system.elf` 读出 `setup_paging` 的地址并打印。这一次链接的例子是 `0x5450`。计数从函数入口开始，停在 `ret` 落到的 `hlt` 上。机器描述在 `labs/i386/gem5/linux-0.11-x86.py`。细节见 [gem5](../gem5.md)。

要看 PG 从 0 变成 1，用 Bochs，不要单步填表的循环。脚本会打印同一地址：

```text
pb 0x5450
c
```

停在第一条 `movl $1024*5,%ecx` 时，PG 仍是 0。单步到 `movl %eax,%cr0` 执行完，再运行 `creg`。

## 该看到什么

Bochs 里 PG 已置位，CR3 为 0。`page 0` 翻译线性地址 0。

gem5 这一次打印过：

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

CPI 是 2.93，来自 9220 次 store 和 1 次 load。322 次数据 miss 里，320 次是写，对应那 20KiB；另外 2 次是读。未命中率是 0.035，一次数据未命中平均 56.11 个周期。store queue 停顿是这个循环的流水线停顿：`stosl` 填队列比内存排空更快。issue queue 停顿是 4。你刚跑的那一次，以脚本打出的数字为准。

同一次运行写出 `m5out/instructions.txt`、`m5out/pipeline.txt` 和 `m5out/pipeline.html`。填表是同一段循环重复很多次，所以文件只留前 32 条微操作和最后 12 条。这一次是 65577 条里的 44 条，末尾是写 CR3、写 CR0，然后返回。65577 和 `microops` 是同一个数。
