# 按章节走一遍

[English](../en/cases.md)

这五章把第 3 步和第 4 步拆开。每一章只问一件事：读一段 1991 年的源码，用已经有的启动命令停下来，看机器状态或执行代价。不另写启动脚本。

地址每次链接都会变。用 `labs/i386/bochs/launch.sh` 或 `labs/i386/gem5/launch.sh` 当场打印的地址。下面各章里的十六进制数是这一次链接的例子。

1. [保护模式](cases/protected-mode.md)。`lmsw` 置上 CR0.PE，GDT 给出平坦的代码段和数据段。
2. [填页表](cases/paging.md)。`setup_paging` 写下前 16MB 的页表。先猜 cache miss，再看 gem5 的计数。
3. [时钟中断](cases/timer.md)。IRQ0 走向量 `0x20`，进入 `_timer_interrupt`。
4. [任务切换](cases/schedule.md)。`_schedule` 用 `ljmp` 换到任务 1 的 TSS。
5. [搬一页](cases/copy-page.md)。`copy_page` 的 `rep movsl` 复制 4KiB，store queue 被填满。

QEMU 里按 F5 仍用来单步源码。Bochs 看描述符表和任务状态。gem5 打印周期、cache miss 和流水线停顿。
