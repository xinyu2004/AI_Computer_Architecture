# Instructions in this kernel

[中文](../zh/instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `mov`, `add`, `sub`, `cmp`, `jmp`, `call`, `push`, and `pop` are not listed. The groups follow the order a reader meets them, from the BIOS entry to a running process. The effect column is the machine change that matters while reading this tree.

## Real mode

The CPU starts here. A segment register holds a paragraph: the linear address is that value shifted left by 4.

| Instruction | Name | Effect |
| --- | --- | --- |
| `int` | Interrupt | Call a BIOS service through a real-mode interrupt vector. `int $0x13` talks to the disk, `int $0x10` to the display, `int $0x15` reports memory size. |
| `rep movsw` | Repeat, move string word | Copy CX words from DS:SI to ES:DI. The boot sector uses it to relocate itself. |
| `ljmp` | Long jump | Load both CS and IP. In real mode this jumps to another segment. The same mnemonic in protected mode reloads a descriptor. |

## Protected mode

| Instruction | Name | Effect |
| --- | --- | --- |
| `cli` | Clear interrupt flag | Clear IF in EFLAGS. Maskable hardware interrupts stay pending. |
| `sti` | Set interrupt flag | Set IF. The timer IRQ can then be delivered. |
| `lmsw` | Load machine status word | Write the low 16 bits of CR0. Bit 0 is PE, protection enable. This 286 instruction cannot clear PE. |
| `lgdt` | Load global descriptor table | Load the GDTR: a byte limit and the linear address of the GDT. |
| `lidt` | Load interrupt descriptor table | Load the IDTR, the same kind of pointer for the IDT. |
| `ljmp` | Long jump | In protected mode, load CS from a selector and refill the hidden segment cache. The far jump after `lmsw` is what makes the following bytes execute as 32-bit code. |

## Segments, tasks, and pages

| Instruction | Name | Effect |
| --- | --- | --- |
| `lldt` | Load local descriptor table | Load the LDTR from a GDT selector. The LDT holds this task's user code and data segments. |
| `ltr` | Load task register | Load the TR from a GDT selector. The TR names the current task's TSS. |
| `lsll` | Load segment limit | Read a descriptor's limit into a register. `get_limit` then adds one, because the hardware limit is the last valid byte. |
| `clts` | Clear task-switched flag | Clear CR0.TS. The coprocessor can be used without a device-not-available fault. |
| `mov` to `cr0` | Move to control register 0 | Write the whole CR0. Bit 31 is PG, paging. This is how paging is turned on. `lmsw` cannot reach that bit. |
| `mov` from `cr2` | Move from control register 2 | Read the linear address that caused the latest page fault. |
| `mov` to `cr3` | Move to control register 3 | Tell the CPU where the page directory is, and drop the translations it has cached. |

## Interrupts and returns

| Instruction | Name | Effect |
| --- | --- | --- |
| `int $0x80` | Interrupt | Enter the kernel through an IDT gate. The CPU pushes EFLAGS, CS, and EIP, then loads CS from the gate. |
| `iret` | Interrupt return | Pop EIP, CS, and EFLAGS. If the popped CS has a less privileged RPL, also pop ESP and SS. |

An interrupt gate (type 14) clears IF on entry. A trap gate (type 15) leaves IF as it was. The system-call gate is a trap gate with DPL 3, so user code may execute `int $0x80`.

## Ports and strings

| Instruction | Name | Effect |
| --- | --- | --- |
| `outb` / `inb` | Output byte / input byte | Write or read one byte on an I/O port. The 8259, the timer, and the keyboard controller are reached this way, not through memory. |
| `cld` / `std` | Clear / set direction flag | `cld` makes string instructions advance the pointers. `std` makes them step backward. |
| `rep movsl` | Repeat, move string doubleword | Copy ECX doublewords from DS:ESI to ES:EDI. `copy_page` copies 1024 of them, which is 4KB. |
| `rep stosl` | Repeat, store string doubleword | Fill ECX doublewords at ES:EDI with EAX. Page-table setup and a freshly allocated page use it. |
| `repne scasb` | Repeat while not equal, scan string byte | Scan ES:EDI for the byte in AL, counting with ECX. The free-page search looks for a zero byte in the page map. |

## Coprocessor

| Instruction | Name | Effect |
| --- | --- | --- |
| `fninit` | Initialize floating point, no wait | Reset the x87 unit. |
| `fstsw` | Store status word | Write the x87 status word. A zero low byte means a coprocessor answered. |
| `fsetpm` | Set protected mode | A 287-only instruction. A 387 ignores these bytes (`DB E4`). |

## Bits named above

| Bit | Name | Meaning |
| --- | --- | --- |
| PE | Protection enable | CR0 bit 0. Set: protected mode. |
| PG | Paging | CR0 bit 31. Set: linear addresses go through the page tables. |
| MP | Monitor coprocessor | CR0 bit 1. A coprocessor is present, so a task switch should trap on the next x87 instruction. |
| EM | Emulate coprocessor | CR0 bit 2. No coprocessor: x87 instructions raise device-not-available. |
| TS | Task switched | CR0 bit 3. Set by a hardware task switch. Cleared by `clts`. |
| IF | Interrupt flag | EFLAGS bit 9. Set: maskable IRQs are delivered. |
| DPL | Descriptor privilege level | The privilege required to use a segment or a gate. |
| RPL | Requested privilege level | The low two bits of a selector, the privilege the program asks for. |
| CPL | Current privilege level | The low two bits of CS. 0 is the kernel, 3 is user code. |
