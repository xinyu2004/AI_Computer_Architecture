# 搬一页

[English](../../en/cases/copy-page.md)

[目录](../cases.md)

## 这一章要看懂什么

写一块共享页时，内核另分配一页并复制 4KiB。`copy_page` 是宏，本体是 1024 个双字的 `rep movsl`。它和 `setup_paging` 的 `stosl` 一样，会把 store queue 填满。一个在填页表，一个在复制一页。

## 读哪里

`src/linux-0.11/mm/memory.c` 第 54 行的 `copy_page`。`-O2` 把它内联进第 225 行的 `un_wp_page`。符号表里没有 `copy_page`。`labs/gem5/launch.sh copy_page` 每次从 `_un_wp_page` 里找出那条 `rep movsl`。

## 怎么启动

在仓库根目录：

```bash
make -C src/linux-0.11
labs/gem5/launch.sh copy_page
```

脚本打印这次找到的地址。这一次链接的例子是 `0x93a3`。计数从这条 `rep movsl` 还没执行时开始，停在下一条指令还没执行的时候。第一次执行是任务 1 写共享页时的那次复制，发生在 `panic("Unable to mount root")` 之前。

[填页表](paging.md) 里的 `stosl` 可以对照着看。

## 该看到什么

gem5 把整段重复记成 2 条指令。工作量在微操作上。这一次打印过：

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

数据 cache 记下 1030 次读和 1013 次写。970 次读 miss 里，61 次拉进新的一行，909 次打中已经在填充的同一行。`get_free_page` 刚把目标页清过，所以写都命中了。store queue 停顿是这个循环的流水线停顿。结束行是 `copy_page returned`。
