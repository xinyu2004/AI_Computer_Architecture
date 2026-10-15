# The same questions, on another CPU

[中文](../zh/course.md)

Linux 0.11 walks these questions first. RV32 answers them again with its own functions. The ARM column stays empty until that kernel exists. RV64 is the register-width difference inside this RISC-V course, not another column. Cycles and pipeline stalls stay in the 80386 [gem5](gem5.md) lab.

![The modes at boot](../../figures/boot-modes.svg)

![One echo hello](../../figures/one-command.svg)

![A byte and a block](../../figures/byte-block.svg)

| Question | 80386 | RV32 | ARM |
| --- | --- | --- | --- |
| Where is the first instruction, and in which mode | `boot/bootsect.s`. F5 stops at `0x7c00` | [Reset](course/rv32.md#boot) | |
| What enters the mode the kernel keeps | [Protected mode](cases/protected-mode.md) | [mret](course/rv32.md#mode) | |
| Which register names the page-table root | [Paging](cases/paging.md) | [satp](course/rv32.md#pages) | |
| What separates user and kernel | After the shell is up, Bochs `sreg` shows CS with the low bits equal to 3. See [Bochs](bochs.md) | [U-mode](course/rv32.md#privilege) | |
| Where the timer cause appears | [Timer interrupt](cases/timer.md) | [scause](course/rv32.md#timer) | |
| Which instruction enters the kernel, and where the call number sits | `system_call` in `kernel/system_call.s`, vector `0x80` | [ecall](course/rv32.md#ecall) | |
| Where the two tasks keep their registers, and which instruction switches | [Task switch](cases/schedule.md) | [swtch](course/rv32.md#switch) | |
| What copies 4KiB, and what makes that code run | [Copy one page](cases/copy-page.md) | [copy_page](course/rv32.md#copy) | |
| How a byte arrives from a device | `kernel/chr_drv/keyboard.S`, vector `0x21` | [uart_getc](course/rv32.md#byte) | |
| How that byte becomes a line | `kernel/chr_drv/tty_io.c` | [read_line](course/rv32.md#line) | |
| How a name finds a block | `fs/namei.c` | [lookup](course/rv32.md#name) | |
| Whether the same block is still in memory the second time | `bread` in `fs/buffer.c` | [bread](course/rv32.md#block) | |
| What wakes the waiter when the disk finishes | `kernel/blk_drv/hd.c`, vector `0x2E` | [virtio_isr](course/rv32.md#disk) | |

Every stop is in [the RV32 pass](course/rv32.md). The third picture is the last five questions.
