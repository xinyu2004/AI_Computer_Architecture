# 在 gem5 里看执行代价

[English](../en/gem5.md)

这是学习的第 4 步。第一条路径是 `boot/head.s` 里的 `setup_paging`，也就是 Bochs 分页那一停所在的函数。这一次统计这个函数的周期、L1 cache miss 和一种流水线停顿。

这一步不改 `src/linux-0.11/`。内核里没有 gem5 的特殊指令。按 F5 仍然调试 QEMU 里的机器。`labs/bochs/launch.sh` 保持原样。

## 这一次在跑什么

gem5 自带的 x86 全系统加载器会拒绝 32 位内核，并改成 64 位长模式的交接。`labs/gem5/linux-0.11-x86.patch` 改的是本地 gem5 树，让 ELF32 内核换一种方式开始：

- 保护模式已经打开，分页关闭。物理地址扩展和长模式保持关闭，因此 `setup_paging` 写下的是 `head.s` 里的 32 位页表。
- 代码用选择子 8，数据用选择子 `0x10`，也就是内核 GDT 里 `_gdt` 处的平坦段。
- CPU 从 `setup_paging` 开始。这个符号每次链接都会移动。`labs/gem5/launch.sh` 从 `tools/system.elf` 读出地址并打印。
- 真正启动时，`setup_paging` 末尾的 `ret` 会进入 `_main`，因为 `after_page_tables` 把这个返回地址压进了栈。这一次在栈上放了另一个返回地址，`ret` 落到物理地址 `0x80000` 的 `hlt`。栈槽在 `0x7F000`。两处都在已加载的内核之上，也在该函数清零的开头五页之外。

CPU 是 gem5 的乱序模型，时钟 1 GHz，带 32KiB 的 L1 指令 cache 和 32KiB 的 L1 数据 cache。cache 行是 64 字节。这些周期来自这个模型，并不是按周期对齐的 80386。指令仍是 1991 年的那个函数。

这一次不引导软盘，不登录，也不启动调度器。完整内核仍在 QEMU 和 Bochs 里跑。

## 怎样检查

在仓库根目录，运行过 `script/bootstrap.sh` 之后：

```bash
make -C src/linux-0.11
labs/gem5/build.sh
labs/gem5/launch.sh
labs/gem5/launch.sh timer
labs/gem5/launch.sh schedule
labs/gem5/launch.sh copy_page
```

`labs/gem5/build.sh` 把 gem5 v25.1.0.1 克隆到 `.gem5/`，打上 `labs/gem5/linux-0.11-x86.patch`，并编译出 `build/X86/gem5.opt`。`.gem5/` 是构建产物，不纳入 git。第一次编译需要一段时间，之后会沿用已有结果。这份补丁改的是 gem5，让这个 32 位内核能启动。它不是内核补丁。

`labs/gem5/launch.sh` 是要运行的命令。它从 `tools/system.elf` 读出符号地址，并打印计数。gem5 自己还要一份机器描述，那就是 `labs/gem5/linux-0.11-x86.py`：一台乱序 x86 CPU、32KiB 的 L1 cache、16MiB 内存、1 GHz 时钟。这一步的每次测量都用这同一台机器。`launch.sh` 传入内核，以及要统计的路径。这个 Python 文件不用单独运行。

`labs/gem5/launch.sh` 接着打印这一次编译的 `setup_paging` 地址，以及 `m5out/stats.txt` 里的计数。这份树上的一次运行打印过：

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

内核重新链接后地址会变。脚本打印的是你刚启动的这一次的地址和计数。停在 0 的计数不会出现在 `stats.txt` 里，脚本把那一行打成 0。`instructions` 计的是 x86 指令。`microops` 计的是流水线重命名的微操作；`stosl` 和 `rep movsl` 会展开成很多条。

`setup_paging` 在物理地址 0 写下五页：页目录和四张页表，共 20KiB。按 64 字节一行，是 320 行。数据 cache 记下 320 次写 miss 和 2 次读 miss。store queue 那一项是重命名因为 store queue 满了而停住的次数。这就是这个循环的流水线停顿：`stosl` 把队列填满的速度超过内存把它排空的速度。每次运行都会打印 serialize stall。在这条路径上，占主导的停顿是 store queue。

完整日志在 `m5out/gem5.log`。里面有一条警告，说 gem5 实例化的 DRAM 芯片比 16MiB 的地址范围更大。L1 miss 的时序仍来自这个 DRAM 模型。结束那一行是 `setup_paging returned`，也就是 `ret` 之后的 `hlt`，所以对 CR3 和 CR0 的写入已经执行过。

## 时钟中断

`labs/gem5/launch.sh timer` 从 `startup_32` 开始。同一份补丁让 CPU 停在 32 位保护模式、分页关闭，并按 `boot/setup.s` 的方式设置 8259：主片向量基址 `0x20`，从片 `0x28`，两片屏蔽都是 `0xFF`。IRQ0 仍接在 8259 上，再送到 CPU。`0x90000` 处的启动参数写明 16MiB 内存，`0x901FC` 处的根设备是 `0x0301`。栈指针取 `_stack_start` 里保存的值。

内核接着执行自己的 `startup_32`，其中包括 `setup_paging`，然后进入 `_main`。这一次没有根磁盘，所以任务 1 的 `setup()` 会调用 `panic("Unable to mount root")`，并停在 `_panic+0x22` 的 `jmp` 上自旋。`sched_init` 写完 PIT 大约 10ms 之后，IRQ0 从这段自旋里进来。处理函数是 1991 年的 `_timer_interrupt`：保存寄存器，递增 `_jiffies`，写 EOI，调用 `_do_timer`，再用 `iret` 返回。被打断的代码在内核里，所以这一拍不会调用 `_schedule`。

计数从 PC 变成 `_timer_interrupt` 时重新开始，此时 `push %ds` 还没执行。计数停在 PC 变回被打断的那条指令、而那条指令还没执行的时候。这一次那条指令是 `0x7a06` 的 `jmp`。一次运行打印过：

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

store queue 没有被填满。这条路径上的流水线停顿是 serialize stall：一条串行指令把流水线排空时，重命名在等。CPU 模型和 `setup_paging` 那一次相同。`m5out/gem5.log` 的结束行是 `timer_interrupt returned`。

QEMU 和 Bochs 仍然引导磁盘并进入 shell。这条 gem5 命令不会。

## schedule

`labs/gem5/launch.sh schedule` 用同一次启动，统计第一次 `_schedule`。任务 0 在 `fork` 返回后从 `pause()` 进入它。这次调用选中任务 1，并对 TSS 选择子 `0x30` 执行 `ljmp`。

计数从 PC 变成 `_schedule` 时重新开始，此时 `push %esi` 还没执行。计数停在 PC 变成任务 1 的第一条指令、而那条指令还没执行的时候。这一次那个线性地址是 `0x4012f0c`：CS 是 `0x0f`，段基址是 `0x4000000`，偏移 `0x12f0c` 是 `_main` 里 `fork` 的返回点。任务切换算在这个窗口里。`_schedule` 末尾的 `ret` 要等任务 0 被切回来才执行。这一次到不了那里。任务 1 继续往下走，随后撞上同一个 `panic("Unable to mount root")`。

一次运行打印过：

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

store queue 没有被填满。流水线停顿是任务切换前后的 serialize stall。结束行是 `schedule switched`。再跑一次，计数相同。

## copy_page

`mm/memory.c` 里的 `copy_page` 是宏，没有符号。它的本体是 1024 个双字、4KiB 的 `rep movsl`。`-O2` 把它内联进 `_un_wp_page`。`labs/gem5/launch.sh copy_page` 找到这条 `rep movsl`，统计它第一次执行。这次链接里这条指令在 `0x93a3`。重新链接后地址会变，脚本打印它找到的地址。

第一次拷贝是任务 1 已经跑起来、写了一页共享页之后，`un_wp_page` 里的写保护复制。计数从 PC 变成这条 `rep movsl`、它还没执行时重新开始，停在 PC 变成下一条指令、那条指令还没执行的时候。

整段重复在 gem5 的指令计数里是 2。工作量出现在微操作上。一次运行打印过：

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

数据 cache 记下 1030 次读和 1013 次写。970 次读 miss 里，61 次拉进新的一行，909 次打中已经在填充的同一行。`get_free_page` 刚把目标页清过，所以写都命中了。store queue 停顿就是这个循环的流水线停顿，和 `setup_paging` 里 `stosl` 的那种一样。结束行是 `copy_page returned`。再跑一次，计数相同。

这次拷贝发生在缺根盘 panic 之前的路上。QEMU 和 Bochs 仍然引导磁盘并进入 shell。

登录、从 BIOS 启动，以及在 F5 里接 gem5，都不在这一步。
