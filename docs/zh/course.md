# 同一批问题，换一台 CPU

[English](../en/course.md)

Linux 0.11 先把这些问题走一遍。RV32 用自己的函数再答一遍。ARMv7 和 AArch64 各有一份内核，各占一列。RV64 是这门 RISC-V 课里的[位宽差异](course/rv64.md)，不单开一列。周期和流水线停顿留在 80386 的 [gem5](gem5.md)。

![启动时的几档](../../figures/boot-modes.svg)

![一次 echo hello](../../figures/one-command.svg)

![一个字节和一块](../../figures/byte-block.svg)

| 问 | 80386 | RV32 | ARMv7 | AArch64 |
| --- | --- | --- | --- | --- |
| 上电后第一条指令在哪一档 | `boot/bootsect.s`，F5 停在 `0x7c00` | [上电](course/rv32.md#boot) | [SVC](course/armv7.md#boot) | [EL1](course/aarch64.md#boot) |
| 谁把它带进内核使用的那一档 | [保护模式](cases/protected-mode.md) | [mret](course/rv32.md#mode) | [留在 SVC](course/armv7.md#mode) | [留在 EL1](course/aarch64.md#mode) |
| 页表的根在哪个寄存器 | [填页表](cases/paging.md) | [satp](course/rv32.md#pages) | [TTBR0](course/armv7.md#pages) | [TTBR0_EL1](course/aarch64.md#pages) |
| 用户态和内核态靠什么分开 | 进 shell 后 Bochs 的 `sreg`，CS 低两位是 3。见 [Bochs](bochs.md) | [U-mode](course/rv32.md#privilege) | [用户态](course/armv7.md#privilege) | [EL0](course/aarch64.md#privilege) |
| 时钟来了，原因码在哪 | [时钟中断](cases/timer.md) | [scause](course/rv32.md#timer) | [GIC 30](course/armv7.md#timer) | [GIC 30](course/aarch64.md#timer) |
| 用户从哪条指令进内核，调用号在哪 | `kernel/system_call.s` 的 `system_call`，向量 `0x80` | [ecall](course/rv32.md#ecall) | [svc，r7](course/armv7.md#ecall) | [svc，x8](course/aarch64.md#ecall) |
| 两个任务的寄存器在哪，哪条指令切换 | [任务切换](cases/schedule.md) | [swtch](course/rv32.md#switch) | [swtch](course/armv7.md#switch) | [swtch](course/aarch64.md#switch) |
| 谁拷走 4KiB，什么让它跑起来 | [搬一页](cases/copy-page.md) | [copy_page](course/rv32.md#copy) | [1024 个字](course/armv7.md#copy) | [512 个字](course/aarch64.md#copy) |
| 一个字节怎样从设备进来 | `kernel/chr_drv/keyboard.S`，向量 `0x21` | [uart_getc](course/rv32.md#byte) | [PL011](course/armv7.md#byte) | [PL011](course/aarch64.md#byte) |
| 这个字节怎样排成一行 | `kernel/chr_drv/tty_io.c` | [read_line](course/rv32.md#line) | [read_line](course/armv7.md#line) | [read_line](course/aarch64.md#line) |
| 一个名字怎样找到一块 | `fs/namei.c` | [lookup](course/rv32.md#name) | [lookup](course/armv7.md#name) | [lookup](course/aarch64.md#name) |
| 同一块第二次还在不在内存里 | `fs/buffer.c` 的 `bread` | [bread](course/rv32.md#block) | [bread](course/armv7.md#block) | [bread](course/aarch64.md#block) |
| 磁盘做完时谁被叫醒 | `kernel/blk_drv/hd.c`，向量 `0x2E` | [virtio_isr](course/rv32.md#disk) | [GIC 48](course/armv7.md#disk) | [GIC 48](course/aarch64.md#disk) |

RV32 的停法在 [RV32 这一遍](course/rv32.md)，ARMv7 在 [ARMv7 这一遍](course/armv7.md)，AArch64 在 [AArch64 这一遍](course/aarch64.md)。第三张图是后五问。
