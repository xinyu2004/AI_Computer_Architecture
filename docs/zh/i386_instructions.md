# 80386 内核里的指令

[English](../en/i386_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `mov`、`add`、`sub`、`cmp`、`jmp`、`call`、`push`、`pop` 不列在这里。分组按阅读顺序，从 BIOS 进入一直到进程跑起来。效果一栏只写读这份代码时要知道的那一次机器变化。

小号括号标在第一列。(386) 是这台 CPU 的机制。(PC) 是 BIOS，或是这块主板。不带括号的一行，这个动作在别的机器上还在，指令名不同。

## 实模式

CPU 从这里开始。段寄存器里放的是段地址，线性地址是它左移 4 位。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `int` <sub>(PC)</sub> | Interrupt，中断 | 经实模式中断向量调用 BIOS。`int $0x13` 访问磁盘，`int $0x10` 访问显示，`int $0x15` 报告内存大小。 |
| `rep movsw` <sub>(386)</sub> | Repeat move string word，重复传送字 | 把 CX 个字从 DS:SI 复制到 ES:DI。引导扇区用它把自己搬走。 |
| `ljmp` <sub>(386)</sub> | Long jump，远跳转 | 同时装入 CS 和 IP。实模式下这是跳到另一个段。保护模式下同一条助记符用来装入一个描述符。 |

## 保护模式

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `cli` | Clear interrupt flag，清除中断标志 | 清除 EFLAGS 里的 IF。可屏蔽硬件中断先挂起。 |
| `sti` | Set interrupt flag，置位中断标志 | 置上 IF。此后时钟 IRQ 才能送达。 |
| `lmsw` <sub>(386)</sub> | Load machine status word，装入机器状态字 | 写 CR0 的低 16 位。第 0 位是 PE，即保护模式使能。这是 286 留下的指令，不能把 PE 清掉。 |
| `lgdt` <sub>(386)</sub> | Load global descriptor table，装入全局描述符表 | 装入 GDTR：一个字节界限，以及 GDT 的线性地址。 |
| `lidt` <sub>(386)</sub> | Load interrupt descriptor table，装入中断描述符表 | 装入 IDTR，形式与 GDTR 相同，指向 IDT。 |
| `ljmp` <sub>(386)</sub> | Long jump，远跳转 | 保护模式下用一个选择子装入 CS，并刷新隐藏的段描述符缓存。`lmsw` 后面的那条远跳转，让随后的字节按 32 位指令执行。 |

## 段、任务和页

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `lldt` <sub>(386)</sub> | Load local descriptor table，装入局部描述符表 | 用一个 GDT 选择子装入 LDTR。LDT 里是这个任务自己的用户代码段和数据段。 |
| `ltr` <sub>(386)</sub> | Load task register，装入任务寄存器 | 用一个 GDT 选择子装入 TR。TR 指向当前任务的 TSS。 |
| `lsll` <sub>(386)</sub> | Load segment limit，装入段界限 | 把描述符里的界限读进寄存器。`get_limit` 再加一，因为硬件里的界限是最后一个有效字节。 |
| `clts` <sub>(386)</sub> | Clear task-switched flag，清除任务已切换标志 | 清除 CR0.TS。之后使用协处理器不会触发设备不可用异常。 |
| `mov` 到 `cr0` | 写入控制寄存器 0 | 写整个 CR0。第 31 位是 PG，即分页。分页是这样打开的。`lmsw` 够不着这一位。 |
| `mov` 从 `cr2` | 读出控制寄存器 2 | 读出最近一次缺页的线性地址。 |
| `mov` 到 `cr3` | 写入控制寄存器 3 | 告诉 CPU 页目录在哪里，并丢掉它已经缓存的翻译。 |

## 中断和返回

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `int $0x80` <sub>(386)</sub> | Interrupt，中断 | 经 IDT 的一门进入内核。CPU 压入 EFLAGS、CS 和 EIP，再从门里装入 CS。 |
| `iret` <sub>(386)</sub> | Interrupt return，中断返回 | 弹出 EIP、CS 和 EFLAGS。弹出的 CS 若 RPL 特权更低，再弹出 ESP 和 SS。 |

在 80386 上，中断门（类型 14）进入时清除 IF。陷阱门（类型 15）保持 IF 原样。系统调用那一门是 DPL 为 3 的陷阱门，所以用户代码可以执行 `int $0x80`。

## 端口和串操作

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `outb` / `inb` <sub>(386)</sub> | Output byte / input byte，输出字节 / 输入字节 | 向一个 I/O 端口写或读一个字节。8259、定时器和键盘控制器走端口，不走内存。 |
| `cld` / `std` <sub>(386)</sub> | Clear / set direction flag，清除 / 置位方向标志 | `cld` 让串指令把指针往前移。`std` 让它们往回退。 |
| `rep movsl` <sub>(386)</sub> | Repeat move string doubleword，重复传送双字 | 把 ECX 个双字从 DS:ESI 复制到 ES:EDI。`copy_page` 复制 1024 个，也就是 4KB。 |
| `rep stosl` <sub>(386)</sub> | Repeat store string doubleword，重复存储双字 | 用 EAX 填充 ES:EDI 处的 ECX 个双字。建立页表和清新分配的页都用它。 |
| `repne scasb` <sub>(386)</sub> | Repeat while not equal, scan string byte，不相等则重复扫描字节 | 在 ES:EDI 里找 AL 中的字节，用 ECX 计数。找空闲页时，在页图里找一个 0 字节。 |

## 协处理器

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `fninit` <sub>(386)</sub> | Initialize floating point, no wait，初始化浮点且不等待 | 复位 x87。 |
| `fstsw` <sub>(386)</sub> | Store status word，存储状态字 | 写出 x87 状态字。低字节为 0 表示协处理器有应答。 |
| `fsetpm` <sub>(386)</sub> | Set protected mode，设置保护模式 | 只对 287 有意义。387 忽略这两个字节（`DB E4`）。 |

## 上面提到的位

| 位 | 全称 | 含义 |
| --- | --- | --- |
| PE <sub>(386)</sub> | Protection enable，保护模式使能 | CR0 第 0 位。置位表示保护模式。 |
| PG | Paging，分页 | CR0 第 31 位。置位后线性地址经过页表。 |
| MP <sub>(386)</sub> | Monitor coprocessor，监视协处理器 | CR0 第 1 位。协处理器存在，任务切换后下一条 x87 指令应陷入。 |
| EM <sub>(386)</sub> | Emulate coprocessor，模拟协处理器 | CR0 第 2 位。没有协处理器，x87 指令触发设备不可用。 |
| TS <sub>(386)</sub> | Task switched，任务已切换 | CR0 第 3 位。硬件任务切换时置位。`clts` 清除它。 |
| IF | Interrupt flag，中断标志 | EFLAGS 第 9 位。置位后可屏蔽 IRQ 才会送达。 |
| DPL <sub>(386)</sub> | Descriptor privilege level，描述符特权级 | 使用一个段或一门所需的特权级。 |
| RPL <sub>(386)</sub> | Requested privilege level，请求特权级 | 选择子的低两位，程序请求的特权级。 |
| CPL <sub>(386)</sub> | Current privilege level，当前特权级 | CS 的低两位。0 是内核，3 是用户代码。 |
