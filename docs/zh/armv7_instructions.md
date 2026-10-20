# ARMv7 内核里的指令

[English](../en/armv7_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `mov`、`add`、`ldr`、`str` 不列在这里。效果一栏只写读 `src/armv7/` 时要知道的那一次机器变化。80386 的名字在[另一页](i386_instructions.md)。RV32 的名字在[另一页](rv32_instructions.md)。

小号括号标在第一列。(ARMv7) 是这台 CPU 的机制。不带括号的一行，这个动作在别的机器上还在，指令名不同。

## 先是 SVC，Hyp 是退路

`_start` 在 SVC 运行。打印出来的 CPSR 低字节是 `0xd3`：SVC，并且 I 和 F 都置着。模式 `0x1a` 才走 Hyp 那条退路。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `cpsid` / `cpsie` | Change processor state, interrupt disable / enable，改处理器状态，关中断 / 开中断 | `cpsid if` 置 I 和 F，IRQ 和 FIQ 都先压着。`cpsid i` 只置 I。`cpsie i` 清 I，提示符上的时钟靠这一下才能进来。 |
| `mrs` / `msr` <sub>(ARMv7)</sub> | Move to register from special / move to special from register，从特殊寄存器读出 / 写入特殊寄存器 | `mrs` 读 CPSR。`msr spsr_cxsf` 写整份保存的程序状态。`msr cpsr_c, #imm` 连模式带中断屏蔽一起写：`0xd3` 是 SVC，`0xd2` 是 IRQ，`0xd7` 是 abort，`0xdb` 是 undefined，`0x1f` 是 System，它和用户态共用栈指针。 |
| `mrc` / `mcr` <sub>(ARMv7)</sub> | Move to ARM register from coprocessor / move to coprocessor from ARM register，从协处理器读到 ARM 寄存器 / 写入协处理器 | 协处理器 15 放着系统寄存器。哪个寄存器由操作数字段决定。这份内核用 `c1` 访问 SCTLR，Hyp 退路里还用它访问 HCR；`c2` 是 TTBR0 和 TTBCR；`c3` 是 DACR；`c12` 是 VBAR；`c14` 是 CNTFRQ、CNTP_CTL，以及 Hyp 的 CNTHCTL；`c5` 和 `c6` 是数据故障的状态和地址；`c8` 用来作废 TLB。 |
| `eret` <sub>(ARMv7)</sub> | Exception return，异常返回 | 从 Hyp 跳到 ELR_hyp，并把 SPSR_hyp 复制进 CPSR。这次启动已经在 SVC，所以这条返回被跳过。 |

<a id="ttbr0"></a>

## 短描述符页表

`vm_on` 用 `mcr` 写 TTBR0、TTBCR 和 DACR，然后置 SCTLR.M。缓存保持关闭。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `dsb` / `isb` | Data synchronization barrier / instruction synchronization barrier，数据同步屏障 / 指令同步屏障 | `dsb` 等到前面的写已经可见。`isb` 等到后面的指令按新的 SCTLR、TTBR0 或 VBAR 重新取指。 |

## 系统调用、IRQ，以及等待

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `svc` <sub>(ARMv7)</sub> | Supervisor call，监管者调用 | 进入 SVC。硬件把 LR_svc 留在下一条指令上。这份内核里 `r7` 选择退出、写、读、执行、列目录、fork、wait、sleep 或按名字查找，`r0` 到 `r2` 是参数。 |
| <a id="movs-pc-lr"></a>`movs pc, lr` <sub>(ARMv7)</sub> | Move to pc, and restore SPSR，写入 pc，并恢复 SPSR | 写 pc 时带上 S，会把 SPSR 复制进 CPSR。`svc` 这样返回，回到用户程序也这样返回。`ldmia ..., pc` 上的 `^` 为 IRQ 返回做同一次复制。IRQ 入口要先把 LR_irq 减 4：硬件把它留在被打断的指令之后四个字节。 |
| `wfi` | Wait for interrupt，等待中断 | 停到有中断挂起。shell 等一行输入时，以及 `sleep` 等毫秒时，都靠下一次时钟把它叫醒。 |
| `hvc` / `smc` <sub>(ARMv7)</sub> | Hypervisor call / secure monitor call，虚拟机监控调用 / 安全监控调用 | `r0` 里是 `0x84000008`，PSCI 的 SYSTEM_OFF。这块板的设备树写的是 `hvc`。`smc` 是另一条通道。 |

## 时钟

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `mrrc` / `mcrr` <sub>(ARMv7)</sub> | Move to two ARM registers from coprocessor / move to coprocessor from two ARM registers，从协处理器读到两个 ARM 寄存器 / 从两个寄存器写入协处理器 | `mrrc p15, 0, c14` 读 64 位的 CNTPCT。`mcrr p15, 2, c14` 写 CNTP_CVAL，也就是比较值。Hyp 退路用 `mcrr` 把 CNTVOFF 清零。 |

中断号是从 GICC_IAR 读出来的，结束中断是写 GICC_EOIR。这两下都是普通的内存访问。这块板以 Secure SVC 进入，内核把每条线标成 Group 1，所以同时置上 AckCtl。这次启动的时钟号是 30，virtio 的号是 48。

## 上面提到的位

| 位 | 全称 | 含义 |
| --- | --- | --- |
| <a id="cpsr"></a>Mode <sub>(ARMv7)</sub> | CPSR 模式域 | 第 4 到 0 位。`0x13` 是 SVC，`0x1a` 是 Hyp，`0x10` 是用户，`0x1f` 是 System。`msr cpsr_c` 写的是第 7 到 0 位，不改条件标志。启动打印的低字节 `0xd3` 是 I、F 和 SVC。 |
| I | IRQ mask，IRQ 屏蔽 | CPSR 第 7 位。置位后 IRQ 先挂着。 |
| F <sub>(ARMv7)</sub> | FIQ mask，FIQ 屏蔽 | CPSR 第 6 位。置位后 FIQ 先挂着。 |
| <a id="sctlr"></a>M | MMU enable，MMU 使能 | SCTLR 第 0 位。置位后短描述符翻译打开。 |
| A | Alignment check，对齐检查 | SCTLR 第 1 位。`vm_on` 清掉。 |
| C | Cache enable，数据缓存 | SCTLR 第 2 位。`vm_on` 清掉。 |
| I | Instruction cache，指令缓存 | SCTLR 第 12 位。`vm_on` 清掉。 |
| TRE | TEX remap enable，TEX 重映射 | SCTLR 第 28 位。`vm_on` 清掉。 |
| AFE | Access flag enable，访问标志 | SCTLR 第 29 位。`vm_on` 清掉。 |
| <a id="ttbcr"></a>EAE <sub>(ARMv7)</sub> | Extended address enable，扩展地址使能 | TTBCR 第 31 位。清掉时 TTBR0 是短描述符的一级表。 |
| N | TTBR0 width，TTBR0 的宽度 | TTBCR 第 2 到 0 位。写成 0，TTBR0 覆盖全部 32 位地址。 |
| PD1 <sub>(ARMv7)</sub> | Translation table walk disable, TTBR1，关闭 TTBR1 的页表遍历 | TTBCR 第 5 位。置位后不再查 TTBR1。 |
| <a id="dacr"></a>DACR 域 0 | Domain access control，域访问控制 | DACR 的低两位写成 `01`，域 0 是 client，页表项里的 AP 才生效。 |
| ENABLE | Timer enable，定时器使能 | CNTP_CTL 第 0 位。置位后用 CNTPCT 和 CNTP_CVAL 比较。 |
| AckCtl <sub>(ARMv7)</sub> | Acknowledge control，应答控制 | GICC_CTLR 第 2 位。置位后，Secure 状态下读 GICC_IAR 能拿到 Group 1 的中断号。 |

CPSR、SCTLR、TTBCR 里上面没写到的位，这份内核不改。
