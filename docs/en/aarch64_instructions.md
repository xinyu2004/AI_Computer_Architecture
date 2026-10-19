# Instructions in the AArch64 kernel

[中文](../zh/aarch64_instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `mov`, `add`, `ldr`, and `str` are not listed. The effect column is the machine change that matters while reading `src/aarch64/`. The 80386 names are on [their own page](i386_instructions.md). The ARMv7 names are on [theirs](armv7_instructions.md).

A smaller parenthetical mark sits in the first column. (AArch64) is a mechanism of this CPU. A row with no parentheses is the same kind of action another machine still does, under another instruction.

## EL1, and an EL2 fallback

`_start` runs at EL1h. CurrentEL reads as 4, which the boot prints. The EL2 path is the fallback for the value 8.

| Instruction | Name | Effect |
| --- | --- | --- |
| `mrs` / `msr` <sub>(AArch64)</sub> | Move to register from system register / move to system register from register | The register name is in the instruction. Boot reads CurrentEL and SCTLR_EL1, and writes VBAR_EL1. The EL2 fallback writes HCR_EL2, CNTHCTL_EL2, CNTVOFF_EL2, SPSR_EL2, and ELR_EL2. A trap reads ESR_EL1 and FAR_EL1. `msr daifset, #2` sets the I mask. `msr daifclr, #2` clears it. The immediate's bit 1 is I. |
| `eret` <sub>(AArch64)</sub> | Exception return | Jump to ELR and copy SPSR into PSTATE. From EL2 the target is EL1h, SPSR `0x3c5`. From EL1 into the shell, SPSR is 0, which is EL0t. An IRQ return uses the same `eret`: ELR_EL1 already holds the restart address, so the handler does not subtract 4. |

## 4KiB pages in TTBR0_EL1

`vm_on` writes MAIR_EL1, TCR_EL1, and TTBR0_EL1, then sets SCTLR_EL1.M. With the MMU still off, memory is treated as Device, so the kernel was built to keep every access aligned.

| Instruction | Name | Effect |
| --- | --- | --- |
| `tlbi vmalle1` <sub>(AArch64)</sub> | TLB invalidate, all entries, EL1 | Drop the translations cached for EL1. It runs after TTBR0_EL1 is written, and again when a user page is remapped. |
| `dsb sy` / `isb` | Data synchronization barrier, full system / instruction synchronization barrier | `dsb sy` waits until the preceding writes, including the TLB invalidate, are finished. `isb` waits until the following instructions are fetched under the new SCTLR_EL1 or VBAR_EL1. |

## System call, IRQ, and the wait

| Instruction | Name | Effect |
| --- | --- | --- |
| `svc` <sub>(AArch64)</sub> | Supervisor call | From EL0 this lands in the vector slot at VBAR_EL1 plus `0x400`. Hardware leaves ELR_EL1 pointing at the next instruction. In this kernel `x8` selects exit, write, read, exec, list, fork, wait, sleep, or lookup by name, and `x0`–`x2` are the arguments. |
| `wfi` | Wait for interrupt | Pause until an interrupt is pending. The shell uses it while waiting for a line, and `sleep` uses it while waiting out the milliseconds. |
| `hvc` / `smc` <sub>(AArch64)</sub> | Hypervisor call / secure monitor call | `x0` holds `0x84000008`, PSCI SYSTEM_OFF. This board's device tree names `hvc`. `smc` is the other conduit. |

The interrupt id is a load from GICC_IAR, and the end of interrupt is a store to GICC_EOIR. The kernel writes the same GICv2 enables as the ARMv7 kernel, including AckCtl. The timer id on this boot is 30. The virtio id is 48.

## Bits named above

| Bit | Name | Meaning |
| --- | --- | --- |
| CurrentEL <sub>(AArch64)</sub> | Current exception level | Bits [3:2]. The value 1 is EL1, so the register reads as 4. The boot prints `0x00000004`. |
| M <sub>(AArch64)</sub> | Exception level and stack | PSTATE bits [3:0]. 5 is EL1h. 0 is EL0t, the SPSR written before `eret` into the shell. SPSR `0x3c5` is EL1h with the DAIF masks set. |
| I | IRQ mask | PSTATE bit 7. Set: IRQ stays pending. `daifclr #2` clears it. |
| M | MMU enable | SCTLR_EL1 bit 0. Set: translation through TTBR0_EL1 is on. |
| WXN <sub>(AArch64)</sub> | Write permission implies XN | SCTLR_EL1 bit 19. Left clear, so EL1 may execute the writable kernel pages. |
| T0SZ <sub>(AArch64)</sub> | TTBR0 size | TCR_EL1 bits [5:0]. 25 means a 39-bit virtual address, and the walk starts at level 1. |
| EPD1 <sub>(AArch64)</sub> | Translation table walk disable, TTBR1 | TCR_EL1 bit 23. Set: TTBR1 is not consulted. |
| EC <sub>(AArch64)</sub> | Exception class | ESR_EL1 bits [31:26]. `0x15` is an `svc` from AArch64. |
