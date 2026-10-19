# AArch64 内核里的指令

[English](../en/aarch64_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `mov`、`add`、`ldr`、`str` 不列在这里。效果一栏只写读 `src/aarch64/` 时要知道的那一次机器变化。80386 的名字在[另一页](i386_instructions.md)。ARMv7 的名字在[另一页](armv7_instructions.md)。

小号括号标在第一列。(AArch64) 是这台 CPU 的机制。不带括号的一行，这个动作在别的机器上还在，指令名不同。

## 先是 EL1，EL2 是退路

`_start` 在 EL1h 运行。CurrentEL 读出来是 4，启动时会打印这个数。值是 8 才走 EL2 那条退路。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `mrs` / `msr` <sub>(AArch64)</sub> | Move to register from system register / move to system register from register，从系统寄存器读出 / 写入系统寄存器 | 寄存器的名字写在指令里。启动读 CurrentEL 和 SCTLR_EL1，并写 VBAR_EL1。EL2 退路写 HCR_EL2、CNTHCTL_EL2、CNTVOFF_EL2、SPSR_EL2 和 ELR_EL2。陷入之后读 ESR_EL1 和 FAR_EL1。`msr daifset, #2` 置上 I 屏蔽。`msr daifclr, #2` 清掉它。这个立即数的第 1 位就是 I。 |
| `eret` <sub>(AArch64)</sub> | Exception return，异常返回 | 跳到 ELR，并把 SPSR 复制进 PSTATE。从 EL2 下去的目标是 EL1h，SPSR 是 `0x3c5`。从 EL1 进入 shell 时 SPSR 是 0，也就是 EL0t。IRQ 返回用的也是这条 `eret`：ELR_EL1 里已经是重新执行的地址，处理函数不再减 4。 |

## TTBR0_EL1 里的 4KiB 页

`vm_on` 写 MAIR_EL1、TCR_EL1 和 TTBR0_EL1，然后置 SCTLR_EL1.M。MMU 还关着的时候，内存按 Device 对待，所以这份内核编译成每次访问都对齐。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `tlbi vmalle1` <sub>(AArch64)</sub> | TLB invalidate, all entries, EL1，作废 EL1 的全部 TLB 项 | 丢掉 EL1 已经缓存的翻译。写完 TTBR0_EL1 之后执行一次，用户页重新映射时再执行一次。 |
| `dsb sy` / `isb` | Data synchronization barrier, full system / instruction synchronization barrier，全系统数据同步屏障 / 指令同步屏障 | `dsb sy` 等到前面的写，包括 TLB 作废，已经完成。`isb` 等到后面的指令按新的 SCTLR_EL1 或 VBAR_EL1 重新取指。 |

## 系统调用、IRQ，以及等待

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `svc` <sub>(AArch64)</sub> | Supervisor call，监管者调用 | 从 EL0 进来，落在 VBAR_EL1 加 `0x400` 的那个槽。硬件把 ELR_EL1 留在下一条指令上。这份内核里 `x8` 选择退出、写、读、执行、列目录、fork、wait、sleep 或按名字查找，`x0` 到 `x2` 是参数。 |
| `wfi` | Wait for interrupt，等待中断 | 停到有中断挂起。shell 等一行输入时，以及 `sleep` 等毫秒时，都靠下一次时钟把它叫醒。 |
| `hvc` / `smc` <sub>(AArch64)</sub> | Hypervisor call / secure monitor call，虚拟机监控调用 / 安全监控调用 | `x0` 里是 `0x84000008`，PSCI 的 SYSTEM_OFF。这块板的设备树写的是 `hvc`。`smc` 是另一条通道。 |

中断号是从 GICC_IAR 读出来的，结束中断是写 GICC_EOIR。内核写的 GICv2 使能和 ARMv7 那份一样，包括 AckCtl。这次启动的时钟号是 30，virtio 的号是 48。

## 上面提到的位

| 位 | 全称 | 含义 |
| --- | --- | --- |
| CurrentEL <sub>(AArch64)</sub> | Current exception level，当前异常级 | 第 3 到 2 位。值 1 表示 EL1，所以寄存器读出来是 4。启动打印 `0x00000004`。 |
| M <sub>(AArch64)</sub> | 异常级和栈的选择 | PSTATE 第 3 到 0 位。5 是 EL1h。0 是 EL0t，也就是 `eret` 进 shell 之前写入的 SPSR。SPSR `0x3c5` 是 EL1h，并且 DAIF 屏蔽都置着。 |
| I | IRQ mask，IRQ 屏蔽 | PSTATE 第 7 位。置位后 IRQ 先挂着。`daifclr #2` 把它清掉。 |
| M | MMU enable，MMU 使能 | SCTLR_EL1 第 0 位。置位后经 TTBR0_EL1 做翻译。 |
| WXN <sub>(AArch64)</sub> | Write permission implies XN，可写则不可执行 | SCTLR_EL1 第 19 位。保持清掉，EL1 才能执行那些可写的内核页。 |
| T0SZ <sub>(AArch64)</sub> | TTBR0 size，TTBR0 的地址宽度 | TCR_EL1 第 5 到 0 位。25 表示虚拟地址是 39 位，页表从第 1 级走起。 |
| EPD1 <sub>(AArch64)</sub> | Translation table walk disable, TTBR1，关闭 TTBR1 的页表遍历 | TCR_EL1 第 23 位。置位后不再查 TTBR1。 |
| EC <sub>(AArch64)</sub> | Exception class，异常类别 | ESR_EL1 第 31 到 26 位。`0x15` 是从 AArch64 来的 `svc`。 |
