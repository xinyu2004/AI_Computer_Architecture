# 在 gem5 里看执行代价

[English](../en/gem5.md)

这是学习的第 4 步。第一条路径是 `boot/head.s` 里的 `setup_paging`，也就是 Bochs 分页那一停所在的函数。这一次统计这个函数的周期、CPI、load 和 store 的组成、L1 未命中率、数据 cache 未命中的平均延迟、已提交的分支，以及重命名阶段的停顿，并把这一段的微操作和流水线图写出来。

这一步不改 `src/linux-0.11/`。内核里没有 gem5 的特殊指令。按 F5 仍然调试 QEMU 里的机器。`labs/i386/bochs/launch.sh` 保持原样。

## 这一次在跑什么

gem5 自带的 x86 全系统加载器会拒绝 32 位内核，并改成 64 位长模式的交接。`labs/i386/gem5/linux-0.11-x86.patch` 改的是本地 gem5 树，让 ELF32 内核换一种方式开始：

- 保护模式已经打开，分页关闭。物理地址扩展和长模式保持关闭，因此 `setup_paging` 写下的是 `head.s` 里的 32 位页表。
- 代码用选择子 8，数据用选择子 `0x10`，也就是内核 GDT 里 `_gdt` 处的平坦段。
- CPU 从 `setup_paging` 开始。这个符号每次链接都会移动。`labs/i386/gem5/launch.sh` 从 `tools/system.elf` 读出地址并打印。
- 真正启动时，`setup_paging` 末尾的 `ret` 会进入 `_main`，因为 `after_page_tables` 把这个返回地址压进了栈。这一次在栈上放了另一个返回地址，`ret` 落到物理地址 `0x80000` 的 `hlt`。栈槽在 `0x7F000`。两处都在已加载的内核之上，也在该函数清零的开头五页之外。

CPU 是 gem5 的乱序模型，时钟 1 GHz，带 32KiB 的 L1 指令 cache 和 32KiB 的 L1 数据 cache。cache 行是 64 字节。这些周期来自这个模型，并不是按周期对齐的 80386。指令仍是 1991 年的那个函数。

这一次不引导软盘，不登录，也不启动调度器。完整内核仍在 QEMU 和 Bochs 里跑。

## 怎样检查

在仓库根目录，运行过 `script/bootstrap.sh` 之后：

```bash
make -C src/linux-0.11
labs/i386/gem5/build.sh
labs/i386/gem5/launch.sh
labs/i386/gem5/launch.sh timer
labs/i386/gem5/launch.sh schedule
labs/i386/gem5/launch.sh copy_page
labs/i386/gem5/launch.sh from START to END
```

`labs/i386/gem5/build.sh` 把 gem5 v25.1.0.1 克隆到 `.gem5/`，打上 `labs/i386/gem5/linux-0.11-x86.patch`，并编译出 `build/X86/gem5.opt`。`.gem5/` 是构建产物，不纳入 git。第一次编译需要一段时间，之后会沿用已有结果。这份补丁改的是 gem5，让这个 32 位内核能启动。它不是内核补丁。

`labs/i386/gem5/launch.sh` 是要运行的命令。它从 `tools/system.elf` 读出符号地址，并打印计数。gem5 自己还要一份机器描述，那就是 `labs/i386/gem5/linux-0.11-x86.py`：一台乱序 x86 CPU、32KiB 的 L1 cache、16MiB 内存、1 GHz 时钟。这一步的每次测量都用这同一台机器。`launch.sh` 传入内核，以及要统计的路径。这个 Python 文件不用单独运行。

`labs/i386/gem5/launch.sh` 接着打印这一次编译的 `setup_paging` 地址，以及 `labs/i386/gem5/m5out/stats.txt` 里的计数。这份树上的一次运行打印过：

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

内核重新链接后地址会变。脚本打印的是你刚启动的这一次的地址和计数。停在 0 的计数不会出现在 `stats.txt` 里，脚本把那一行打成 0。每一行都和 `numCycles` 是同一段。`instructions` 计的是 x86 指令。`microops` 计的是流水线重命名的微操作；`stosl` 和 `rep movsl` 会展开成很多条。`cpi` 是周期数除以这些 x86 指令。`loads` 和 `stores` 是已提交的访存。访问次数和未命中率给未命中次数一个分母。`L1 D miss cycles` 是 `overallAvgMissLatency` 除以 `system.cpu_clk_domain.clock`（1 GHz 时是 1000 个 tick），所以它是数据 cache 一次未命中的平均周期，不是 `numCycles` 的另一份拷贝。`branches` 和 `branch mispredicts` 计已提交的分支。`issue-queue stalls` 是重命名因为发射队列满了而停住的次数。`labs/i386/gem5/m5out/stats.txt` 里其余的计数仍留在文件中。

`setup_paging` 在物理地址 0 写下五页：页目录和四张页表，共 20KiB。按 64 字节一行，是 320 行。数据 cache 记下 320 次写 miss 和 2 次读 miss。store queue 那一项是重命名因为 store queue 满了而停住的次数。这就是这个循环的流水线停顿：`stosl` 把队列填满的速度超过内存把它排空的速度。每次运行都会打印 serialize stall。在这条路径上，占主导的停顿是 store queue。

完整日志在 `labs/i386/gem5/m5out/gem5.log`。里面有一条警告，说 gem5 实例化的 DRAM 芯片比 16MiB 的地址范围更大。L1 miss 的时序仍来自这个 DRAM 模型。结束那一行是 `setup_paging returned`，也就是 `ret` 之后的 `hlt`，所以对 CR3 和 CR0 的写入已经执行过。

## 指令文本和流水线

同一次运行还会写出 `labs/i386/gem5/m5out/instructions.txt`、`labs/i386/gem5/m5out/pipeline.txt` 和 `labs/i386/gem5/m5out/pipeline.html`。记录从计数清零开始，到这个窗口结束为止，窗口之前的启动过程不在这两个文件里。

`instructions.txt` 按程序顺序列出已提交的微操作。几条微操作可以属于同一条 x86 指令，地址写成 `pc.upc`。`timer` 和 `schedule` 保留整个窗口。`paging` 保留前 32 条微操作和最后 12 条，这样对 CR3 和 CR0 的写入还在文件里。`copy_page` 保留 `rep movsl` 循环的前 32 条。这一次文件里的说明是「65577 条里的前 32 条和最后 12 条」以及「6151 条里的前 32 条」。这个数字和 `microops` 那一行相同。对不上的话 `launch.sh` 会停下来。

`pipeline.txt` 为这份列表里的每条微操作画一行。时间向右走。`f` 是取指，`d` 译码，`n` 重命名，`p` 分发，`i` 发射，`c` 完成，`r` 退休。一个点是在该级停留或等待下一级的一个周期。一行宽 72 个周期，并且对齐到这条微操作自己的取指时刻，所以大约在启动 10ms 之后才开始的时钟中断窗口，打出来仍是短短一行。超过 72 个周期的微操作续到后面几行。运行停在一个 PC 上，和计数用的是同一个窗口。每个字母是流水线的一级，图在运行结束之后写出。

时钟中断的文件从 `push %ds` 开始：

```text
0x00006ee4.0   PUSH_S : rdsel   t1d, DS
0x00006ee4.1   PUSH_S : st   t1d, SS:[t0d + esp + 0xfffffffffffffffc]
0x00006ee4.2   PUSH_S : subi   esp, esp, 0x4
```

这三条微操作是一起取指的：

```text
[....................fdn.ic.r............................................]-(    10070352000) 0x00006ee4.0 PUSH_S                    [  28942374]
[....................fdn.pic.r...........................................]-(    10070352000) 0x00006ee4.1 PUSH_S                    [  28942375]
[....................fdn.ic..r...........................................]-(    10070352000) 0x00006ee4.2 PUSH_S                    [  28942376]
```

`labs/i386/gem5/m5out/pipeline.html` 是同一份列表，用浏览器打开，一条微操作一行。每条色条的左端都是取指。七种颜色依次是取指、译码、重命名、分发、发射、完成、退休。色块的宽度是停在这一级或等待下一级的周期，所有行用同一个比例，所以一次 miss 会明显更长。右边的数字是这条微操作的周期数。长循环和文本文件用同一段切片。

文本文件一直排到 `iret`。后面有一行是写 DS 的微操作，在指令文件里是 `wrdl DS`。分发已经完成，发射还在等：

```text
[................................fdn.p...............................ic.r]-(    10070352000) 0x00006ef1.9 MOV_S_R                   [  28942444]
```

## 时钟中断

`labs/i386/gem5/launch.sh timer` 从 `startup_32` 开始。同一份补丁让 CPU 停在 32 位保护模式、分页关闭，并按 `boot/setup.s` 的方式设置 8259：主片向量基址 `0x20`，从片 `0x28`，两片屏蔽都是 `0xFF`。IRQ0 仍接在 8259 上，再送到 CPU。`0x90000` 处的启动参数写明 16MiB 内存，`0x901FC` 处的根设备是 `0x0301`。栈指针取 `_stack_start` 里保存的值。

内核接着执行自己的 `startup_32`，其中包括 `setup_paging`，然后进入 `_main`。这一次没有根磁盘，所以任务 1 的 `setup()` 会调用 `panic("Unable to mount root")`，并停在 `_panic+0x22` 的 `jmp` 上自旋。`sched_init` 写完 PIT 大约 10ms 之后，IRQ0 从这段自旋里进来。处理函数是 1991 年的 `_timer_interrupt`：保存寄存器，递增 `_jiffies`，写 EOI，调用 `_do_timer`，再用 `iret` 返回。被打断的代码在内核里，所以这一拍不会调用 `_schedule`。

计数从 PC 变成 `_timer_interrupt` 时重新开始，此时 `push %ds` 还没执行。计数停在 PC 变回被打断的那条指令、而那条指令还没执行的时候。这一次那条指令是 `0x7a06` 的 `jmp`。一次运行打印过：

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

store queue 没有被填满。这条路径上的流水线停顿是 serialize stall：一条串行指令把流水线排空时，重命名在等。CPI 是 15.2。处理函数提交了 32 次 load 和 14 次 store，32 次分支里有 16 次预测错。issue queue 停顿是 6。CPU 模型和 `setup_paging` 那一次相同。`labs/i386/gem5/m5out/gem5.log` 的结束行是 `timer_interrupt returned`。

QEMU 和 Bochs 仍然引导磁盘并进入 shell。这条 gem5 命令不会。

## schedule

`labs/i386/gem5/launch.sh schedule` 用同一次启动，统计第一次 `_schedule`。任务 0 在 `fork` 返回后从 `pause()` 进入它。这次调用选中任务 1，并对 TSS 选择子 `0x30` 执行 `ljmp`。

计数从 PC 变成 `_schedule` 时重新开始，此时 `push %esi` 还没执行。计数停在 PC 变成任务 1 的第一条指令、而那条指令还没执行的时候。这一次那个线性地址是 `0x4012f0c`：CS 是 `0x0f`，段基址是 `0x4000000`，偏移 `0x12f0c` 是 `_main` 里 `fork` 的返回点。任务切换算在这个窗口里。`_schedule` 末尾的 `ret` 要等任务 0 被切回来才执行。这一次到不了那里。任务 1 继续往下走，随后撞上同一个 `panic("Unable to mount root")`。

一次运行打印过：

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

store queue 没有被填满。CPI 是 1.65。这一段提交了 162 次 load 和 20 次 store，281 次分支里有 27 次预测错。issue queue 停顿是 138，旁边是任务切换前后的 274 次 serialize stall。结束行是 `schedule switched`。再跑一次，计数相同。

## copy_page

`mm/memory.c` 里的 `copy_page` 是宏，没有符号。它的本体是 1024 个双字、4KiB 的 `rep movsl`。`-O2` 把它内联进 `_un_wp_page`。`labs/i386/gem5/launch.sh copy_page` 找到这条 `rep movsl`，统计它第一次执行。这次链接里这条指令在 `0x93a3`。重新链接后地址会变，脚本打印它找到的地址。

第一次拷贝是任务 1 已经跑起来、写了一页共享页之后，`un_wp_page` 里的写保护复制。计数从 PC 变成这条 `rep movsl`、它还没执行时重新开始，停在 PC 变成下一条指令、那条指令还没执行的时候。

整段重复在 gem5 的指令计数里是 1，就是这条 `rep movsl`。工作量出现在微操作上。一次运行打印过：

```text
copy_page at 0x93a3
cycles                      2565   system.cpu.numCycles
instructions                   1   system.cpu.commitStats0.numInsts
microops                    6151   system.cpu.commitStats0.numOps
cpi                  2565.000000   system.cpu.cpi
loads                       1024   system.cpu.commitStats0.numLoadInsts
stores                      1024   system.cpu.commitStats0.numStoreInsts
L1 I accesses                  2   system.cpu.icache.overallAccesses::total
L1 I miss rate                 0   system.cpu.icache.overallMissRate::total
L1 I misses                    0   system.cpu.icache.overallMisses::total
L1 D accesses               2043   system.cpu.dcache.overallAccesses::total
L1 D miss rate          0.474792   system.cpu.dcache.overallMissRate::total
L1 D misses                  970   system.cpu.dcache.overallMisses::total
L1 D miss cycles           51.44   system.cpu.dcache.overallAvgMissLatency::total
branches                    1024   system.cpu.branchPred.committed_0::total
branch mispredicts            1   system.cpu.branchPred.mispredicted_0::total
store-queue stalls           917   system.cpu.rename.SQFullEvents
issue-queue stalls           147   system.cpu.rename.IQFullEvents
serialize stalls              20   system.cpu.rename.status::SerializeStall
```

CPI 是 2565，因为它除以的是 x86 指令数，而这里的指令数停在 1。6151 个微操作才是工作量。这一段提交了 1024 次 load 和 1024 次 store。数据未命中率是 0.47，一次数据未命中平均 51.44 个周期。数据 cache 记下 1030 次读和 1013 次写。970 次读 miss 里，61 次拉进新的一行，909 次打中已经在填充的同一行。`get_free_page` 刚把目标页清过，所以写都命中了。store queue 停顿就是这个循环的流水线停顿，和 `setup_paging` 里 `stosl` 的那种一样。issue queue 停顿是 147。结束行是 `copy_page returned`。再跑一次，计数相同。

这次拷贝发生在缺根盘 panic 之前的路上。QEMU 和 Bochs 仍然引导磁盘并进入 shell。

## 自己指定一段

```bash
labs/i386/gem5/launch.sh from START to END
```

`START` 和 `END` 是 `tools/system.elf` 里的符号，或者 `0x6ee4` 这样的十六进制地址。符号从你刚链接的内核里读出。十六进制地址按写下来的值用。一个名字既是符号又像十六进制时，按符号算。

启动和 `timer` 相同：从 `startup_32` 开始，没有根磁盘，同一台乱序 CPU。计数在 PC 变成 `START`、那条指令还没执行时清零，在 PC 变成 `END`、那条指令还没执行时停下。`END` 是窗口外的第一条指令。文件留下窗口里全部已提交的微操作，条数和 `microops` 对不上时 `launch.sh` 会停下来。

- 两个地址在开跑前就定好。跑起来才算出来的终点仍用已有的参数。`_schedule` 里 `ljmp` 之后任务 1 的第一条指令，用 `launch.sh schedule`。
- `START` 必须在这次启动里执行到。只有挂上根磁盘之后才会走到的代码，这次到不了。运行会停在 tick 上限，`launch.sh` 失败。
- `END` 是 `START` 之后会执行到的一条指令的地址。指令中间的地址对不上。如果这个 PC 只在 `START` 之前出现，窗口不会结束。
- 自己指定的一段整段保留。`setup_paging` 和 `rep movsl` 很长，仍用原来的参数，那两个会切片。
- 原来的四个参数不变。

登录、从 BIOS 启动，以及在 F5 里接 gem5，都不在这一步。
