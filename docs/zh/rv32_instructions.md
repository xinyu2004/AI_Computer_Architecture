# RV32 内核里的指令

[English](../en/rv32_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `add`、`lw`、`sw`、`jal` 不列在这里。效果一栏只写读 `src/rv32/` 时要知道的那一次机器变化。80386 的名字在[另一页](i386_instructions.md)。

小号括号标在第一列。(RV32) 是这台 CPU 的机制。不带括号的一行，这个动作在 80386 内核里还在，指令名不同。

## 先是机器模式，再是监管者模式

`_start` 在 M-mode 运行。`mret` 进入 S-mode。用户程序的 `ecall` 再进入 S-mode。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `mret` <sub>(RV32)</sub> | Machine return，机器模式返回 | 跳到 `mepc`，并进入 `mstatus.MPP` 指定的模式。`_start` 用它进入 S-mode。 |
| `ecall` <sub>(RV32)</sub> | Environment call，环境调用 | 陷入更高一级的特权模式。用户程序用它进入内核。`a7` 选择退出、写、读、执行、列目录、fork、wait、sleep 或按名字查找。 |
| `sret` <sub>(RV32)</sub> | Supervisor return，监管者返回 | 跳到 `sepc`，并把 `SPIE` 复制回 `SIE`。陷阱处理函数这样返回。 |
| `sfence.vma` | Supervisor fence, virtual memory，监管者虚拟内存屏障 | 丢掉 CPU 已经缓存的地址翻译。写 `satp` 的前后各执行一次。 |
| `csrr` / `csrw` | CSR read / CSR write，读 / 写控制状态寄存器 | 在通用寄存器和控制寄存器之间搬一个值。M-mode 用到 `pmpaddr0`、`pmpcfg0`、`medeleg`、`mideleg`、`mstatus`、`mepc`、`mcounteren` 和 `menvcfgh`。S-mode 用到 `satp`、`stvec`、`sepc`、`scause`、`sie`、`sstatus`、`sscratch`、`time`、`timeh`、`stimecmp` 和 `stimecmph`。 |
| `wfi` | Wait for interrupt，等待中断 | 停到有中断挂起。shell 等一行输入时，以及 `sleep` 等毫秒时，都靠下一次时钟把它叫醒。 |

## 上面提到的位

| 位 | 全称 | 含义 |
| --- | --- | --- |
| `satp.MODE` | Address-translation mode，地址翻译模式 | RV32 上 `satp` 的第 31 位。1 表示 Sv32，两级页表，每页 4KiB。 |
| SIE | Supervisor interrupt enable，监管者中断使能 | `sstatus` 第 1 位。置位后可以接受监管者中断。 |
| STIE | Supervisor timer interrupt enable，监管者时钟中断使能 | `sie` 第 5 位。置位后时钟中断可以进来。 |
| 中断位 <sub>(RV32)</sub> | 这是中断，不是异常 | `scause` 的第 31 位。置位且低位等于 5，就是监管者时钟中断。 |
