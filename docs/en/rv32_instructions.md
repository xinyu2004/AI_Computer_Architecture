# Instructions in the RV32 kernel

[中文](../zh/rv32_instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `add`, `lw`, `sw`, and `jal` are not listed. The effect column is the machine change that matters while reading `src/rv32/`. The 80386 names are on [their own page](i386_instructions.md).

A smaller parenthetical mark sits in the first column. (RV32) is a mechanism of this CPU. A row with no parentheses is the same kind of action the 80386 kernel still does, under another instruction.

## Machine mode, then supervisor mode

`_start` runs in M-mode. `mret` enters S-mode. A user program's `ecall` then enters S-mode again.

| Instruction | Name | Effect |
| --- | --- | --- |
| `mret` <sub>(RV32)</sub> | Machine return | Jump to `mepc` and enter the mode named by `mstatus.MPP`. `_start` uses it to enter S-mode. |
| `ecall` <sub>(RV32)</sub> | Environment call | Trap to the next more privileged mode. A user program uses it to enter the kernel. `a7` selects exit, write, read, exec, list, fork, wait, sleep, or lookup by name. |
| `sret` <sub>(RV32)</sub> | Supervisor return | Jump to `sepc` and copy `SPIE` back into `SIE`. This is how a trap handler returns. |
| `sfence.vma` | Supervisor fence, virtual memory | Drop translations the CPU has cached. It runs before and after the write to `satp`. |
| `csrr` / `csrw` | CSR read / CSR write | Move a value between a register and a control register. M-mode uses `pmpaddr0`, `pmpcfg0`, `medeleg`, `mideleg`, `mstatus`, `mepc`, `mcounteren`, and `menvcfgh`. S-mode uses `satp`, `stvec`, `sepc`, `scause`, `sie`, `sstatus`, `sscratch`, `time`, `timeh`, `stimecmp`, and `stimecmph`. |
| `wfi` | Wait for interrupt | Pause until an interrupt is pending. The shell uses it while waiting for a line, and `sleep` uses it while waiting out the milliseconds. The next timer tick is what resumes them. |

## Bits named above

| Bit | Name | Meaning |
| --- | --- | --- |
| `satp.MODE` | Address-translation mode | Bit 31 of `satp` on RV32. 1 selects Sv32, a two-level table of 4KiB pages. |
| SIE | Supervisor interrupt enable | `sstatus` bit 1. Set: supervisor interrupts can be taken. |
| STIE | Supervisor timer interrupt enable | `sie` bit 5. Set: the timer interrupt is allowed through. |
| interrupt bit <sub>(RV32)</sub> | Interrupt, not an exception | Bit 31 of `scause`. Set, with the low bits equal to 5: the supervisor timer. |

## What moves on RV64

`src/rv64` uses the instructions above. `sd` and `ld` are the 64-bit store and load. The control transfers stay on this page. These fields move.

| Field | Where it sits on RV64 |
| --- | --- |
| `satp.MODE` | Bits 63:60. The value 8 is Sv39, a three-level table of 4KiB pages. On RV32 the mode is bit 31. |
| interrupt bit | Bit 63 of `scause`. Set, with the low bits equal to 5, the value at the prompt is `0x8000000000000005`. On RV32 that bit is bit 31. |
| `time` | One register. On RV32 the counter is `time` and `timeh`. |
| `stimecmp` | One register, CSR `0x14d`. On RV32 the compare value is `stimecmp` and `stimecmph`. |
| STCE | Bit 63 of `menvcfg`, CSR `0x30a`. On RV32 that bit is bit 31 of `menvcfgh`. |
