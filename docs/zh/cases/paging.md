# 填页表

[English](../../en/cases/paging.md)

[目录](../cases.md)

## 这一章要看懂什么

`setup_paging` 在物理地址 0 写下页目录和四张页表，共 20KiB，把前 16MB 做成恒等映射，然后置上 CR0.PG。这些写会把 store queue 填满。周期来自 gem5 的乱序模型，时钟 1 GHz，L1 各 32KiB，行长 64 字节。

## 读哪里

`src/linux-0.11/boot/head.s` 第 199 行的 `setup_paging`。它先用 `stosl` 清零五页，再填四张页表，然后写 CR3 和 CR0，最后 `ret`。

## 先猜

20KiB、64 字节一行，是 320 行。数据 cache 的写 miss 应落在 320 这一档。

## 怎么启动

在仓库根目录，gem5 已经由 `labs/gem5/build.sh` 编好之后：

```bash
make -C src/linux-0.11
labs/gem5/launch.sh
```

脚本从 `tools/system.elf` 读出 `setup_paging` 的地址并打印。这一次链接的例子是 `0x5450`。计数从函数入口开始，停在 `ret` 落到的 `hlt` 上。机器描述在 `labs/gem5/linux-0.11-x86.py`。细节见 [gem5](../gem5.md)。

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
cycles                 36047   system.cpu.numCycles
instructions           12307   system.cpu.commitStats0.numInsts
microops               65577   system.cpu.commitStats0.numOps
L1 I misses               12   system.cpu.icache.overallMisses::total
L1 D misses              322   system.cpu.dcache.overallMisses::total
store-queue stalls     23832   system.cpu.rename.SQFullEvents
serialize stalls         172   system.cpu.rename.status::SerializeStall
```

322 次数据 miss 里，320 次是写，对应那 20KiB；另外 2 次是读。store queue 停顿是这个循环的流水线停顿：`stosl` 填队列比内存排空更快。你刚跑的那一次，以脚本打出的数字为准。
