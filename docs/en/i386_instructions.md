# Instructions in the 80386 kernel

[中文](../zh/i386_instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `mov`, `add`, `sub`, `cmp`, `jmp`, `call`, `push`, and `pop` are not listed. The groups follow the order a reader meets them, from the BIOS entry to a running process. The effect column is the machine change that matters while reading this tree.

A smaller parenthetical mark sits in the first column. (386) is a mechanism of this CPU. (PC) is the BIOS, or a device on this board. A row with no parentheses is the same kind of action another machine still does, under another instruction.

## Real mode

The CPU starts here. One access uses a pair, not a segment register alone. Instruction fetch is CS:IP, a string source is DS:SI, its destination is ES:DI, and the stack is SS:SP. The linear address is that pair's segment shifted left by 4, plus its own offset. Both halves are 16 bits and the address bus is 20 bits, which is why the shift is 4, through `0xFFFFF`. The BIOS enters at CS:IP = `0000:7C00`. The `rep movsw` runs with DS:SI = `07C0:0000` and ES:DI = `9000:0000`.

| Instruction | Name | Effect |
| --- | --- | --- |
| <a id="int"></a>`int` <sub>(PC)</sub> | Interrupt | Call a BIOS service through a real-mode interrupt vector. `int $0x13` talks to the disk, `int $0x10` to the display, `int $0x15` reports memory size. |
| <a id="rep-movsw"></a>`rep movsw` <sub>(386)</sub> | Repeat, move string word | Copy CX words from DS:SI to ES:DI. The boot sector uses it to relocate itself. |
| <a id="ljmp-real"></a>`ljmp` <sub>(386)</sub> | Long jump | Load both CS and IP. In real mode this jumps to another segment. The same mnemonic in protected mode reloads a descriptor; see the [protected-mode far jump](#ljmp). |

## Protected mode

| Instruction | Name | Effect |
| --- | --- | --- |
| <a id="cli"></a>`cli` | Clear interrupt flag | Clear IF in [EFLAGS](#eflags). Maskable hardware interrupts stay pending. |
| <a id="sti"></a>`sti` | Set interrupt flag | Set IF. The timer IRQ can then be delivered. |
| <a id="lmsw"></a>`lmsw` <sub>(386)</sub> | Load machine status word | Write the source into the low 16 bits of [CR0](#cr0). In `setup.s` AX is 1, so those 16 bits become `0x0001`: PE is 1, and MP, EM, TS, and ET are 0. This 286 instruction cannot clear PE. |
| <a id="lgdt"></a>`lgdt` <sub>(386)</sub> | Load global descriptor table | Read 6 bytes from memory into GDTR: a 16-bit limit plus a 32-bit base. CS and CR0 stay as they were. In `setup.s` those 6 bytes are [gdt_48](#gdt48). |
| <a id="lidt"></a>`lidt` <sub>(386)</sub> | Load interrupt descriptor table | Load the IDTR from 6 bytes of the same shape, naming the IDT. |
| <a id="ljmp"></a>`ljmp` <sub>(386)</sub> | Long jump | In protected mode, load CS from a selector and refill the hidden segment cache. The far jump after `lmsw` is what makes the following bytes execute as 32-bit code. |

## Segments, tasks, and pages

| Instruction | Name | Effect |
| --- | --- | --- |
| `lldt` <sub>(386)</sub> | Load local descriptor table | Load the LDTR from a GDT selector. The LDT holds this task's user code and data segments. |
| `ltr` <sub>(386)</sub> | Load task register | Load the TR from a GDT selector. The TR names the current task's TSS. |
| `lsll` <sub>(386)</sub> | Load segment limit | Read a descriptor's limit into a register. `get_limit` then adds one, because the hardware limit is the last valid byte. |
| <a id="clts"></a>`clts` <sub>(386)</sub> | Clear task-switched flag | Clear TS in [CR0](#cr0). The coprocessor can be used without a device-not-available fault. |
| <a id="mov-cr0"></a>`mov` to `cr0` | Move to control register 0 | Write the whole CR0. Bit 31 is PG, paging. This is how paging is turned on. `lmsw` cannot reach that bit. |
| <a id="mov-cr2"></a>`mov` from `cr2` | Move from control register 2 | Read the linear address that caused the latest page fault. |
| <a id="mov-cr3"></a>`mov` to `cr3` | Move to control register 3 | Tell the CPU where the page directory is, and drop the translations it has cached. |

## Interrupts and returns

| Instruction | Name | Effect |
| --- | --- | --- |
| <a id="int80"></a>`int $0x80` <sub>(386)</sub> | Interrupt | Enter the kernel through an IDT gate. The CPU pushes EFLAGS, CS, and EIP, then loads CS from the gate. |
| <a id="iret"></a>`iret` <sub>(386)</sub> | Interrupt return | Pop EIP, CS, and EFLAGS. If the popped CS has a less privileged RPL, also pop ESP and SS. |

On the 80386, an interrupt gate (type 14) clears IF on entry. A trap gate (type 15) leaves IF as it was. The system-call gate is a trap gate with DPL 3, so user code may execute `int $0x80`.

## Ports and strings

| Instruction | Name | Effect |
| --- | --- | --- |
| `outb` / `inb` <sub>(386)</sub> | Output byte / input byte | Write or read one byte on an I/O port. The 8259, the timer, and the keyboard controller are reached this way, not through memory. |
| `cld` / `std` <sub>(386)</sub> | Clear / set direction flag | `cld` makes string instructions advance the pointers. `std` makes them step backward. |
| <a id="rep-movsl"></a>`rep movsl` <sub>(386)</sub> | Repeat, move string doubleword | Copy ECX doublewords from DS:ESI to ES:EDI. `copy_page` copies 1024 of them, which is 4KB. |
| `rep stosl` <sub>(386)</sub> | Repeat, store string doubleword | Fill ECX doublewords at ES:EDI with EAX. Page-table setup and a freshly allocated page use it. |
| `repne scasb` <sub>(386)</sub> | Repeat while not equal, scan string byte | Scan ES:EDI for the byte in AL, counting with ECX. The free-page search looks for a zero byte in the page map. |

## Coprocessor

| Instruction | Name | Effect |
| --- | --- | --- |
| `fninit` <sub>(386)</sub> | Initialize floating point, no wait | Reset the x87 unit. |
| `fstsw` <sub>(386)</sub> | Store status word | Write the x87 status word. A zero low byte means a coprocessor answered. |
| `fsetpm` <sub>(386)</sub> | Set protected mode | A 287-only instruction. A 387 ignores these bytes (`DB E4`). |

<a id="cr0"></a>

## CR0

Thirty-two bits. On the 386 the named bits are 0, 1, 2, 3, 4, and 31. Bits 5 through 30 are reserved: they read as 0 and a write leaves them 0. The rows marked 486 are names that later occupied some of those reserved numbers. This 0.11 image does not set them.

`lmsw` writes the low 16 bits from its source. In `setup.s` AX is 1, so the low 16 bits become `0x0001`. `head.s` then keeps bits 0, 4, and 31 with the mask `0x80000011` and ORs 2 to set MP. With no coprocessor it clears MP and sets EM. Paging is the later OR of `0x80000000`. How the registers change is in the [mechanism pictures](mech.md#segments).

![CR0](../../figures/cr0.svg)

| Bit | Name | In this kernel |
| --- | --- | --- |
| <a id="pe"></a>0 PE | Protection enable | `lmsw` writes it as 1. Selectors are then read as protected-mode selectors. |
| <a id="mp"></a>1 MP | Monitor coprocessor | `head.s` ORs 2 to set it. With a coprocessor, the next x87 instruction traps while TS is 1. Cleared when no coprocessor answers. |
| <a id="em"></a>2 EM | Emulate coprocessor | Set: an x87 instruction raises device-not-available. `check_x87` sets it when no coprocessor answers. |
| <a id="ts"></a>3 TS | Task switched | The CPU sets it on a hardware task switch. `clts` clears it. Together with MP it decides whether the next x87 instruction traps. |
| <a id="et"></a>4 ET | Extension type | 1 means the coprocessor is a 387. The `head.s` mask keeps this bit. After `lmsw` writes `0x0001`, it is 0. |
| <a id="ne"></a>5 NE | Numeric error | A 486 bit. Reserved on the 386. A comment in `head.s` names it. The code leaves it 0. |
| 6–15 | Reserved | Left 0 on the 386 and in this image. |
| <a id="wp"></a>16 WP | Write protect | A 486 bit. Set: a ring-0 write to a read-only page faults. The comment names it. The code leaves it 0. |
| 17 | Reserved | Left 0. |
| 18 AM | Alignment mask | A 486 bit. Set, and with EFLAGS.AC set, an unaligned access faults. This image leaves it 0. |
| 19–28 | Reserved | Left 0. |
| 29 NW | Not write-through | A 486 bit. Selects cache write policy. This image leaves it 0. |
| 30 CD | Cache disable | A 486 bit. Set: cache lines are not filled. This image leaves it 0. |
| <a id="pg"></a>31 PG | Paging | `setup_paging` ORs `0x80000000` to set it. Linear addresses then walk the page tables. `lmsw` cannot reach this bit. |

<a id="eflags"></a>

## EFLAGS

Thirty-two bits. The arithmetic flags are CF, PF, AF, ZF, SF, and OF, at bits 0, 2, 4, 6, 7, and 11. They are not a single run of bits 0 through 7. Bit 1 always reads as 1. An interrupt gate clears IF on entry. `iret` writes the whole register back from the stack.

![EFLAGS](../../figures/eflags.svg)

| Bit | Name | In this kernel |
| --- | --- | --- |
| <a id="cf"></a>0 CF | Carry | Carry or borrow out of the top bit of an add or subtract. |
| 1 | Reserved | Reads as 1. |
| <a id="pf"></a>2 PF | Parity | Set when the low 8 bits contain an even number of 1 bits. |
| 3 | Reserved | Left 0. |
| <a id="af"></a>4 AF | Auxiliary carry | Carry from the low 4 bits into bit 4. Decimal adjust uses it. |
| 5 | Reserved | Left 0. |
| <a id="zf"></a>6 ZF | Zero | Set when the result is 0. |
| <a id="sf"></a>7 SF | Sign | A copy of the top bit of the result. |
| <a id="tf"></a>8 TF | Trap | Set: the CPU traps after each instruction. The boot path leaves it 0. |
| <a id="if"></a>9 IF | Interrupt enable | Set: maskable IRQs are delivered. `cli` clears it, `sti` sets it. An interrupt gate clears it on entry. |
| <a id="df"></a>10 DF | Direction | Set: string instructions walk toward lower addresses. `std` sets it, `cld` clears it. |
| <a id="of"></a>11 OF | Overflow | Set when a signed result does not fit. |
| <a id="iopl"></a>12–13 IOPL | I/O privilege level | Code numerically below this level faults on `in`, `out`, `cli`, and `sti`. |
| <a id="nt"></a>14 NT | Nested task | The CPU sets it when a task gate enters another task. `iret` then returns through the previous TSS. |
| 15 | Reserved | Left 0. |
| <a id="rf"></a>16 RF | Resume | Present on the 386. Set: the instruction returned to does not raise the same debug exception again. The boot path does not write it. |
| <a id="vm"></a>17 VM | Virtual 8086 mode | Set: that task forms addresses as a real-mode segment:offset pair. The boot path leaves it 0. |
| 18–31 | Reserved | Reserved on the 386. AC (bit 18) and ID (bit 21) are names from later CPUs. This image leaves them 0. |

<a id="gdt48"></a>

## GDTR and gdt_48

GDTR and IDTR are each a 16-bit limit plus a 32-bit base, not a bitfield. `lgdt` copies 6 bytes from memory into GDTR. CS, DS, and CR0 stay as they were. The hidden descriptor cache refills on the next write to a segment register, or on a far jump.

Before `lgdt`, `setup.s` sets DS to `0x9020`, so `lgdt gdt_48` reads the linear address `0x90200` plus the offset of `gdt_48` inside `setup.s`. Those 6 bytes are:

| Bytes | Contents |
| --- | --- |
| 0–1 | Limit `0x800`. 256 descriptors, 8 bytes each. |
| 2–3 | Base bits 15:0, written as `512+gdt`. `setup` lives at `0x90200`, which is 512 bytes past `0x90000`, so adding 512 turns the offset of `gdt` into an offset from `0x90000`. |
| 4–5 | Base bits 31:16, the value `0x0009`. |

The base those fields form is `0x90000 + 512 + the offset of gdt`, the linear address of the label `gdt`. The GDT itself is the descriptor array at that label. `gdt_48` only tells GDTR where that array is.

`lidt idt_48` at the same point writes both the IDTR limit and the IDTR base as 0, so the IDT pointer is empty until `head.s` executes `lidt` again. LDTR and TR are selectors. The descriptor they name gives the base and limit of the LDT or the TSS.

<a id="rings"></a>

## Four privilege levels

0 has the most authority and 3 the least. This kernel's CS is only 0 or 3. Rings 1 and 2 exist and stay unused.

| Name | Where it sits | Meaning |
| --- | --- | --- |
| <a id="cpl"></a>CPL | The low two bits of CS | The level of the instruction now running. 0 is the kernel, 3 is user code. |
| <a id="dpl"></a>DPL | The segment or gate descriptor | The level required to use it. The system-call gate has DPL 3, so ring 3 may execute `int $0x80`. The timer gate has DPL 0, and hardware delivers it. |
| <a id="rpl"></a>RPL | The low two bits of a selector | The level this request claims. |

For a data segment, the numerically larger of CPL and RPL must be less than or equal to the DPL. For an `int` instruction, the gate DPL must be greater than or equal to CPL.

![Privilege](../../figures/rings.svg)
