# Instructions in the RV32 kernel

[中文](../zh/rv32_instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `add`, `lw`, `sw`, and `jal` are not listed. The effect column is the machine change that matters while reading `src/rv32/`. The 80386 names are on [their own page](i386_instructions.md).

A smaller parenthetical mark sits in the first column. (RV32) is a mechanism of this CPU. A row with no parentheses is the same kind of action the 80386 kernel still does, under another instruction.

## Supervisor mode

OpenSBI leaves the hart in S-mode. These instructions talk to that mode, or ask M-mode to do something S-mode is not allowed to do.

| Instruction | Name | Effect |
| --- | --- | --- |
| `ecall` <sub>(RV32)</sub> | Environment call | Trap to the next more privileged mode. Here that is OpenSBI. `a7` selects the call: set the timer, or shut down. |
| `sret` <sub>(RV32)</sub> | Supervisor return | Jump to `sepc` and copy `SPIE` back into `SIE`. This is how a trap handler returns. |
| `sfence.vma` | Supervisor fence, virtual memory | Drop translations the CPU has cached. It runs before and after the write to `satp`. |
| `csrr` / `csrw` | CSR read / CSR write | Move a value between a register and a control register. This kernel uses `satp`, `stvec`, `sepc`, `scause`, `sie`, `sstatus`, `time`, and `timeh`. |
| `wfi` | Wait for interrupt | Pause until an interrupt is pending. The boot uses it to wait for the one timer tick. |

## Bits named above

| Bit | Name | Meaning |
| --- | --- | --- |
| `satp.MODE` | Address-translation mode | Bit 31 of `satp` on RV32. 1 selects Sv32, a two-level table of 4KiB pages. |
| SIE | Supervisor interrupt enable | `sstatus` bit 1. Set: supervisor interrupts can be taken. |
| STIE | Supervisor timer interrupt enable | `sie` bit 5. Set: the timer interrupt is allowed through. |
| interrupt bit <sub>(RV32)</sub> | Interrupt, not an exception | Bit 31 of `scause`. Set, with the low bits equal to 5: the supervisor timer. |
