# 80386 内核里的指令

[English](../en/i386_instructions.md)

一行一条名字本身看不出作用的指令。普通的 `mov`、`add`、`sub`、`cmp`、`jmp`、`call`、`push`、`pop` 不列在这里。分组按阅读顺序，从 BIOS 进入一直到进程跑起来。效果一栏只写读这份代码时要知道的那一次机器变化。

小号括号标在第一列。(386) 是这台 CPU 的机制。(PC) 是 BIOS，或是这块主板。不带括号的一行，这个动作在别的机器上还在，指令名不同。

## 实模式

CPU 从这里开始。一次访问用一对寄存器，不是单独一个段寄存器。取指令是 CS:IP，串传送的源是 DS:SI、目的是 ES:DI，栈是 SS:SP。线性地址是这一对里的段寄存器左移 4 位，再加上它自己的偏移。段和偏移都是 16 位，地址总线是 20 位，所以是左移 4 位，结果到 `0xFFFFF`。BIOS 进入时是 CS:IP = `0000:7C00`。`rep movsw` 时 DS:SI = `07C0:0000`，ES:DI = `9000:0000`。

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| <a id="int"></a>`int` <sub>(PC)</sub> | Interrupt，中断 | 经实模式中断向量调用 BIOS。`int $0x13` 访问磁盘，`int $0x10` 访问显示，`int $0x15` 报告内存大小。 |
| <a id="rep-movsw"></a>`rep movsw` <sub>(386)</sub> | Repeat move string word，重复传送字 | 把 CX 个字从 DS:SI 复制到 ES:DI。引导扇区用它把自己搬走。 |
| <a id="ljmp-real"></a>`ljmp` <sub>(386)</sub> | Long jump，远跳转 | 同时装入 CS 和 IP。实模式下这是跳到另一个段。保护模式下同一条助记符用来装入一个描述符，见 [保护模式的远跳转](#ljmp)。 |

## 保护模式

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| <a id="cli"></a>`cli` | Clear interrupt flag，清除中断标志 | 清除 [EFLAGS](#eflags) 里的 IF。可屏蔽硬件中断先挂起。 |
| <a id="sti"></a>`sti` | Set interrupt flag，置位中断标志 | 置上 IF。此后时钟 IRQ 才能送达。 |
| <a id="lmsw"></a>`lmsw` <sub>(386)</sub> | Load machine status word，装入机器状态字 | 把源操作数写入 [CR0](#cr0) 的低 16 位。`setup.s` 里 AX 是 1，所以这 16 位变成 `0x0001`：PE 为 1，MP、EM、TS、ET 为 0。这是 286 留下的指令，不能把 PE 清掉。 |
| <a id="lgdt"></a>`lgdt` <sub>(386)</sub> | Load global descriptor table，装入全局描述符表 | 从内存读 6 个字节装入 GDTR：16 位界限，加 32 位基址。不改 CS，也不改 CR0。`setup.s` 里这 6 字节是 [gdt_48](#gdt48)。 |
| <a id="lidt"></a>`lidt` <sub>(386)</sub> | Load interrupt descriptor table，装入中断描述符表 | 装入 IDTR，6 字节的形式与 GDTR 相同，指向 IDT。 |
| <a id="ljmp"></a>`ljmp` <sub>(386)</sub> | Long jump，远跳转 | 保护模式下用一个选择子装入 CS，并刷新隐藏的段描述符缓存。`lmsw` 后面的那条远跳转，让随后的字节按 32 位指令执行。 |

## 段、任务和页

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `lldt` <sub>(386)</sub> | Load local descriptor table，装入局部描述符表 | 用一个 GDT 选择子装入 LDTR。LDT 里是这个任务自己的用户代码段和数据段。 |
| `ltr` <sub>(386)</sub> | Load task register，装入任务寄存器 | 用一个 GDT 选择子装入 TR。TR 指向当前任务的 TSS。 |
| `lsll` <sub>(386)</sub> | Load segment limit，装入段界限 | 把描述符里的界限读进寄存器。`get_limit` 再加一，因为硬件里的界限是最后一个有效字节。 |
| <a id="clts"></a>`clts` <sub>(386)</sub> | Clear task-switched flag，清除任务已切换标志 | 清除 [CR0](#cr0) 的 TS。之后使用协处理器不会触发设备不可用异常。 |
| <a id="mov-cr0"></a>`mov` 到 `cr0` | 写入控制寄存器 0 | 写整个 CR0。第 31 位是 PG，即分页。分页是这样打开的。`lmsw` 够不着这一位。 |
| <a id="mov-cr2"></a>`mov` 从 `cr2` | 读出控制寄存器 2 | 读出最近一次缺页的线性地址。 |
| <a id="mov-cr3"></a>`mov` 到 `cr3` | 写入控制寄存器 3 | 告诉 CPU 页目录在哪里，并丢掉它已经缓存的翻译。 |

## 中断和返回

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| <a id="int80"></a>`int $0x80` <sub>(386)</sub> | Interrupt，中断 | 经 IDT 的一门进入内核。CPU 压入 EFLAGS、CS 和 EIP，再从门里装入 CS。 |
| <a id="iret"></a>`iret` <sub>(386)</sub> | Interrupt return，中断返回 | 弹出 EIP、CS 和 EFLAGS。弹出的 CS 若 RPL 特权更低，再弹出 ESP 和 SS。 |

在 80386 上，中断门（类型 14）进入时清除 IF。陷阱门（类型 15）保持 IF 原样。系统调用那一门是 DPL 为 3 的陷阱门，所以用户代码可以执行 `int $0x80`。

## 端口和串操作

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `outb` / `inb` <sub>(386)</sub> | Output byte / input byte，输出字节 / 输入字节 | 向一个 I/O 端口写或读一个字节。8259、定时器和键盘控制器走端口，不走内存。 |
| `cld` / `std` <sub>(386)</sub> | Clear / set direction flag，清除 / 置位方向标志 | `cld` 让串指令把指针往前移。`std` 让它们往回退。 |
| <a id="rep-movsl"></a>`rep movsl` <sub>(386)</sub> | Repeat move string doubleword，重复传送双字 | 把 ECX 个双字从 DS:ESI 复制到 ES:EDI。`copy_page` 复制 1024 个，也就是 4KB。 |
| `rep stosl` <sub>(386)</sub> | Repeat store string doubleword，重复存储双字 | 用 EAX 填充 ES:EDI 处的 ECX 个双字。建立页表和清新分配的页都用它。 |
| `repne scasb` <sub>(386)</sub> | Repeat while not equal, scan string byte，不相等则重复扫描字节 | 在 ES:EDI 里找 AL 中的字节，用 ECX 计数。找空闲页时，在页图里找一个 0 字节。 |

## 协处理器

| 指令 | 全称 | 效果 |
| --- | --- | --- |
| `fninit` <sub>(386)</sub> | Initialize floating point, no wait，初始化浮点且不等待 | 复位 x87。 |
| `fstsw` <sub>(386)</sub> | Store status word，存储状态字 | 写出 x87 状态字。低字节为 0 表示协处理器有应答。 |
| `fsetpm` <sub>(386)</sub> | Set protected mode，设置保护模式 | 只对 287 有意义。387 忽略这两个字节（`DB E4`）。 |

<a id="cr0"></a>

## CR0

32 位。386 上有名字的是第 0、1、2、3、4、31 位。第 5 位到第 30 位在 386 上保留，读出来是 0，写进去也要保持 0。下表里标 486 的那些名字，是后来占了保留位的编号。这份 0.11 映像不置它们。

`lmsw` 把低 16 位写成 AX。`setup.s` 里 AX 是 1，所以低 16 位变成 `0x0001`。`head.s` 再用掩码 `0x80000011` 留下第 0、4、31 位，或上 2 置 MP。没有协处理器时清掉 MP 并置 EM。分页是后来或上 `0x80000000`。寄存器怎么变，见 [机制图](mech.md#segments)。

![CR0](../../figures/cr0-zh.svg)

| 位 | 名字 | 这份内核 |
| --- | --- | --- |
| <a id="pe"></a>0 PE | Protection enable，保护模式使能 | `lmsw` 把它写成 1。为 1 时按保护模式解释选择子。 |
| <a id="mp"></a>1 MP | Monitor coprocessor，监视协处理器 | `head.s` 或上 2 把它置 1。有协处理器时，TS 为 1 的下一条 x87 指令陷入。没有协处理器时被清掉。 |
| <a id="em"></a>2 EM | Emulate coprocessor，模拟协处理器 | 为 1 时 x87 指令触发设备不可用。`check_x87` 发现没有协处理器才置它。 |
| <a id="ts"></a>3 TS | Task switched，任务已切换 | 硬件任务切换时 CPU 置 1。`clts` 清掉。和 MP 一起决定下一条 x87 指令要不要陷入。 |
| <a id="et"></a>4 ET | Extension type，扩展类型 | 1 表示协处理器是 387。`head.s` 的掩码留下这一位。`lmsw` 写过 `0x0001` 之后，它是 0。 |
| <a id="ne"></a>5 NE | Numeric error，数值错误 | 486 才有。386 上这是保留位。`head.s` 的注释提到它，代码不置。 |
| 6–15 | 保留 | 386 和这份映像都保持 0。 |
| <a id="wp"></a>16 WP | Write protect，写保护 | 486 才有。为 1 时，ring 0 写只读页也会出错。注释提到它，代码不置。 |
| 17 | 保留 | 保持 0。 |
| 18 AM | Alignment mask，对齐屏蔽 | 486 才有。为 1 并且 EFLAGS.AC 为 1 时，未对齐访问出错。这份映像不置。 |
| 19–28 | 保留 | 保持 0。 |
| 29 NW | Not write-through，不通写 | 486 才有。管缓存写策略。这份映像不置。 |
| 30 CD | Cache disable，关闭缓存 | 486 才有。为 1 时不按缓存行填充。这份映像不置。 |
| <a id="pg"></a>31 PG | Paging，分页 | `setup_paging` 或上 `0x80000000` 把它置 1。为 1 时线性地址先查页表。`lmsw` 写不到这一位。 |

<a id="eflags"></a>

## EFLAGS

32 位。算术标志是 CF、PF、AF、ZF、SF、OF，分别在第 0、2、4、6、7、11 位，不是连着的第 0 到第 7 位。第 1 位读出来恒为 1。中断门进入时清 IF。`iret` 从栈上把整个寄存器写回去。

![EFLAGS](../../figures/eflags-zh.svg)

| 位 | 名字 | 这份内核 |
| --- | --- | --- |
| <a id="cf"></a>0 CF | Carry flag，进位 | 加减法向更高一位的进位或借位。 |
| 1 | 保留 | 读出来是 1。 |
| <a id="pf"></a>2 PF | Parity flag，奇偶 | 结果低 8 位里 1 的个数为偶数时为 1。 |
| 3 | 保留 | 保持 0。 |
| <a id="af"></a>4 AF | Auxiliary carry，辅助进位 | 低 4 位向第 4 位的进位。十进制调整用它。 |
| 5 | 保留 | 保持 0。 |
| <a id="zf"></a>6 ZF | Zero flag，零 | 结果为 0 时为 1。 |
| <a id="sf"></a>7 SF | Sign flag，符号 | 等于结果的最高位。 |
| <a id="tf"></a>8 TF | Trap flag，陷阱 | 为 1 时每条指令结束都陷入。引导代码不置。 |
| <a id="if"></a>9 IF | Interrupt flag，中断允许 | 为 1 时可屏蔽 IRQ 才会送达。`cli` 清，`sti` 置。中断门进入时 CPU 清它。 |
| <a id="df"></a>10 DF | Direction flag，方向 | 为 1 时串指令从高地址往低地址走。`std` 置，`cld` 清。 |
| <a id="of"></a>11 OF | Overflow flag，溢出 | 有符号结果超出范围时为 1。 |
| <a id="iopl"></a>12–13 IOPL | I/O privilege level，I/O 特权级 | 数值低于这个级别的代码执行 `in`、`out`、`cli`、`sti` 会出错。 |
| <a id="nt"></a>14 NT | Nested task，嵌套任务 | 通过任务门进入另一个任务时 CPU 置 1。`iret` 据此返回上一任务的 TSS。 |
| 15 | 保留 | 保持 0。 |
| <a id="rf"></a>16 RF | Resume flag，恢复 | 386 就有。为 1 时，返回后的那条指令不再重复产生调试异常。引导代码不写。 |
| <a id="vm"></a>17 VM | Virtual 8086 mode，虚拟 8086 | 为 1 时这条任务按实模式的段:偏移取址。引导代码不置。 |
| 18–31 | 保留 | 386 上保留。AC（第 18 位）和 ID（第 21 位）是更晚的 CPU 才有的名字。这份映像不置。 |

<a id="gdt48"></a>

## GDTR 和 gdt_48

GDTR、IDTR 各是 16 位界限加 32 位基址，不是按位解释的状态字。`lgdt` 只把内存里的 6 个字节抄进 GDTR。CS、DS 和 CR0 都不变。隐藏的描述符缓存要等下一次写段寄存器，或一条远跳转，才按新表重装。

`setup.s` 在 `lgdt` 之前把 DS 设成 `0x9020`，所以 `lgdt gdt_48` 读的是线性地址 `0x90200` 加上 `gdt_48` 在 `setup.s` 里的偏移。那 6 个字节的组成是：

| 字节 | 内容 |
| --- | --- |
| 0–1 | 界限 `0x800`。256 个描述符，每个 8 字节。 |
| 2–3 | 基址的低 16 位，汇编写成 `512+gdt`。`setup` 放在 `0x90200`，也就是 `0x90000` 往后 512 字节，加上 512 就把 `gdt` 的偏移换成从 `0x90000` 算起。 |
| 4–5 | 基址的高 16 位，`0x0009`。 |

合起来的基址是 `0x90000 + 512 + gdt 的偏移`，也就是标号 `gdt` 的线性地址。GDT 本体是那个标号处的描述符数组。`gdt_48` 只是告诉 GDTR 去哪里找这张表。

`lidt idt_48` 的 6 字节在同一时刻把 IDTR 的界限和基址都写成 0，等于先把 IDT 指空。`head.s` 以后再 `lidt` 一次，指向真正的 IDT。LDTR 和 TR 是选择子，选中的描述符给出 LDT 和 TSS 的基址与界限。

<a id="rings"></a>

## 四个特权级

0 最有权，3 最没有。这份内核的 CS 只有 0 和 3。1 和 2 存在，这里不用。

| 名字 | 在哪 | 含义 |
| --- | --- | --- |
| <a id="cpl"></a>CPL | CS 的低两位 | 当前这条指令的级别。0 是内核，3 是用户代码。 |
| <a id="dpl"></a>DPL | 段或门的描述符 | 使用它所需的级别。系统调用那扇门的 DPL 是 3，所以 ring 3 可以执行 `int $0x80`。时钟门的 DPL 是 0，由硬件送入。 |
| <a id="rpl"></a>RPL | 选择子的低两位 | 这次请求自己声明的级别。 |

访问数据段时，CPL 和 RPL 里数值较大的那个要小于等于 DPL。执行 `int` 时，门的 DPL 要大于等于 CPL。

![特权级](../../figures/rings-zh.svg)
