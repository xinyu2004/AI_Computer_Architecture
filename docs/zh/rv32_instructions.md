# RV32 内核里的指令

[English](../en/rv32_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `add`、`lw`、`sw`、`jal` 不列在这里。效果一栏只写读 `src/rv32/` 时要知道的那一次机器变化。80386 的名字在[另一页](i386_instructions.md)。

小号括号标在第一列。(RV32) 是这台 CPU 的机制。不带括号的一行，这个动作在 80386 内核里还在，指令名不同。

## 先是机器模式，再是监管者模式

`_start` 在 M-mode 运行。`mret` 进入 S-mode。用户程序的 `ecall` 再进入 S-mode。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| <a id="mret"></a>`mret` <sub>(RV32)</sub> | Machine return，机器模式返回 | 跳到 `mepc`，并进入 `mstatus.MPP` 指定的模式。`_start` 用它进入 S-mode。 |
| <a id="ecall"></a>`ecall` <sub>(RV32)</sub> | Environment call，环境调用 | 陷入更高一级的特权模式。用户程序用它进入内核。`a7` 选择退出、写、读、执行、列目录、fork、wait、sleep 或按名字查找。 |
| <a id="sret"></a>`sret` <sub>(RV32)</sub> | Supervisor return，监管者返回 | 跳到 `sepc`，并把 `SPIE` 复制回 `SIE`。陷阱处理函数这样返回。 |
| `sfence.vma` | Supervisor fence, virtual memory，监管者虚拟内存屏障 | 丢掉 CPU 已经缓存的地址翻译。写 `satp` 的前后各执行一次。 |
| `csrr` / `csrw` | CSR read / CSR write，读 / 写控制状态寄存器 | 在通用寄存器和控制寄存器之间搬一个值。M-mode 用到 `pmpaddr0`、`pmpcfg0`、`medeleg`、`mideleg`、`mstatus`、`mepc`、`mcounteren` 和 `menvcfgh`。S-mode 用到 `satp`、`stvec`、`sepc`、`scause`、`sie`、`sstatus`、`sscratch`、`time`、`timeh`、`stimecmp` 和 `stimecmph`。 |
| `wfi` | Wait for interrupt，等待中断 | 停到有中断挂起。shell 等一行输入时，以及 `sleep` 等毫秒时，都靠下一次时钟把它叫醒。 |

## 上面提到的位

| 位 | 全称 | 含义 |
| --- | --- | --- |
| <a id="satp"></a>`satp.MODE` | Address-translation mode，地址翻译模式 | `satp` 第 31 位。1 表示 Sv32，两级页表，每页 4KiB。第 30 到 22 位是 ASID，这份内核写成 0。第 21 到 0 位是页表根的物理页号，也就是根地址右移 12 位。 |
| <a id="sstatus"></a>SIE | Supervisor interrupt enable，监管者中断使能 | `sstatus` 第 1 位。置位后可以接受监管者中断。 |
| SPIE | Supervisor previous interrupt enable，先前的中断使能 | `sstatus` 第 5 位。`sret` 把它复制回 SIE。回到用户态之前把它置 1。 |
| SPP | Supervisor previous privilege，先前的特权级 | `sstatus` 第 8 位。清掉时 `sret` 进入 U-mode。 |
| SUM | Supervisor user memory，监管者访问用户内存 | `sstatus` 第 18 位。置位后 S-mode 在系统调用里可以读写用户页。 |
| STIE | Supervisor timer interrupt enable，监管者时钟中断使能 | `sie` 第 5 位。置位后时钟中断可以进来。 |
| <a id="scause"></a>中断位 <sub>(RV32)</sub> | 这是中断，不是异常 | `scause` 的第 31 位。置位且低位等于 5，就是监管者时钟；低位等于 9 是外部中断。低位等于 8 且第 31 位为 0，是 U-mode 的 `ecall`。 |

`satp`、`sstatus`、`scause` 里上面没写到的位，这份内核不改，保持进入 S-mode 时的值。RV64 上 MODE 和中断位的位置见下一节。

## RV64 上搬动的位

`src/rv64` 用的是上面这些指令。`sd` 和 `ld` 是 64 位的存储和加载。控制转移仍在这一页。下面这些域换了位置。

| 域 | 在 RV64 上的位置 |
| --- | --- |
| `satp.MODE` | 第 63 到 60 位。值 8 是 Sv39，三级页表，每页 4KiB。RV32 上这个模式是第 31 位。 |
| 中断位 | `scause` 的第 63 位。置位且低位等于 5 时，提示符上的值是 `0x8000000000000005`。RV32 上这一位是第 31 位。 |
| `time` | 一个寄存器。RV32 上计数器是 `time` 和 `timeh`。 |
| `stimecmp` | 一个寄存器，CSR `0x14d`。RV32 上比较值是 `stimecmp` 和 `stimecmph`。 |
| STCE | `menvcfg` 的第 63 位，CSR `0x30a`。RV32 上这一位是 `menvcfgh` 的第 31 位。 |
