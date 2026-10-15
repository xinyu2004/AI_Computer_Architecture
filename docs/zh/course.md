# 同一批问题，换一台 CPU

[English](../en/course.md)

Linux 0.11 先把这些问题走一遍。RV32 用自己的函数再答一遍。ARM 还没有内核，那一列空着。RV64 是这门 RISC-V 课里的位宽差异，不单开一列。周期和流水线停顿留在 80386 的 [gem5](gem5.md)。

![启动时的几档](../../figures/boot-modes.svg)

![一次 echo hello](../../figures/one-command.svg)

![一个字节和一块，还没做](../../figures/byte-block.svg)

| 问 | 80386 | RV32 | ARM |
| --- | --- | --- | --- |
| 上电后第一条指令在哪一档 | `boot/bootsect.s`，F5 停在 `0x7c00` | [上电](course/rv32.md#boot) | |
| 谁把它带进内核使用的那一档 | [保护模式](cases/protected-mode.md) | [mret](course/rv32.md#mode) | |
| 页表的根在哪个寄存器 | [填页表](cases/paging.md) | [satp](course/rv32.md#pages) | |
| 用户态和内核态靠什么分开 | 进 shell 后 Bochs 的 `sreg`，CS 低两位是 3。见 [Bochs](bochs.md) | [U-mode](course/rv32.md#privilege) | |
| 时钟来了，原因码在哪 | [时钟中断](cases/timer.md) | [scause](course/rv32.md#timer) | |
| 用户从哪条指令进内核，调用号在哪 | `kernel/system_call.s` 的 `system_call`，向量 `0x80` | [ecall](course/rv32.md#ecall) | |
| 两个任务的寄存器在哪，哪条指令切换 | [任务切换](cases/schedule.md) | [swtch](course/rv32.md#switch) | |
| 谁拷走 4KiB，什么让它跑起来 | [搬一页](cases/copy-page.md) | [copy_page](course/rv32.md#copy) | |
| 一个字节怎样从设备进来 | `kernel/chr_drv/keyboard.S`，向量 `0x21` | 后半，还没做 | |
| 这个字节怎样排成一行 | `kernel/chr_drv/tty_io.c` | 后半，还没做 | |
| 一个名字怎样找到一块 | `fs/namei.c` | 后半，还没做。现在的平面目录能按名字找到文件 | |
| 同一块第二次还在不在内存里 | `fs/buffer.c` 的 `bread` | 后半，还没做 | |
| 磁盘做完时谁被叫醒 | `kernel/blk_drv/hd.c`，向量 `0x2E` | 后半，还没做 | |

后五问的 RV32 格对应上面第三张图。前八问的停法在 [RV32 这一遍](course/rv32.md)。
