# Instructions in the ARMv7 kernel

[中文](../zh/armv7_instructions.md)

One row is one instruction whose name does not say what it does. Ordinary `mov`, `add`, `ldr`, and `str` are not listed. The effect column is the machine change that matters while reading `src/armv7/`. The 80386 names are on [their own page](i386_instructions.md). The RV32 names are on [theirs](rv32_instructions.md).

A smaller parenthetical mark sits in the first column. (ARMv7) is a mechanism of this CPU. A row with no parentheses is the same kind of action another machine still does, under another instruction.

## SVC, and a Hyp fallback

`_start` runs in SVC. The low byte of the printed CPSR is `0xd3`: SVC, with I and F set. The Hyp path is the fallback for mode `0x1a`.

| Instruction | Name | Effect |
| --- | --- | --- |
| `cpsid` / `cpsie` | Change processor state, interrupt disable / enable | `cpsid if` sets I and F, so IRQ and FIQ stay masked. `cpsid i` sets I. `cpsie i` clears I, which is what lets the timer run at the prompt. |
| `mrs` / `msr` <sub>(ARMv7)</sub> | Move to register from special / move to special from register | `mrs` reads CPSR. `msr spsr_cxsf` writes the whole saved program state. `msr cpsr_c, #imm` writes the mode field together with the interrupt masks: `0xd3` is SVC, `0xd2` is IRQ, `0xd7` is abort, `0xdb` is undefined, and `0x1f` is System, which shares the user stack pointer. |
| `mrc` / `mcr` <sub>(ARMv7)</sub> | Move to ARM register from coprocessor / move to coprocessor from ARM register | Coprocessor 15 holds the system registers. The opcode fields name the register. This kernel uses `c1` for SCTLR and, in the Hyp fallback, HCR; `c2` for TTBR0 and TTBCR; `c3` for DACR; `c12` for VBAR; `c14` for CNTFRQ, CNTP_CTL, and the Hyp CNTHCTL; `c5` and `c6` for the data-fault status and address; `c8` to invalidate the TLB. |
| `eret` <sub>(ARMv7)</sub> | Exception return | From Hyp, jump to ELR_hyp and copy SPSR_hyp into CPSR. This boot is already SVC, so that return is skipped. |

## Short-descriptor pages

`vm_on` writes TTBR0, TTBCR, and DACR through `mcr`, then sets SCTLR.M. Caches stay off.

| Instruction | Name | Effect |
| --- | --- | --- |
| `dsb` / `isb` | Data synchronization barrier / instruction synchronization barrier | `dsb` waits until the preceding writes are visible. `isb` waits until the following instructions are fetched under the new SCTLR, TTBR0, or VBAR. |

## System call, IRQ, and the wait

| Instruction | Name | Effect |
| --- | --- | --- |
| `svc` <sub>(ARMv7)</sub> | Supervisor call | Enter SVC mode. Hardware leaves LR_svc pointing at the next instruction. In this kernel `r7` selects exit, write, read, exec, list, fork, wait, sleep, or lookup by name, and `r0`–`r2` are the arguments. |
| `movs pc, lr` <sub>(ARMv7)</sub> | Move to pc, and restore SPSR | The S bit on a write to pc copies SPSR into CPSR. That is the return from `svc` and the return to a user program. The `^` on `ldmia ..., pc` does the same copy for the IRQ return. IRQ entry subtracts 4 from LR_irq first: the hardware left it four bytes past the interrupted instruction. |
| `wfi` | Wait for interrupt | Pause until an interrupt is pending. The shell uses it while waiting for a line, and `sleep` uses it while waiting out the milliseconds. |
| `hvc` / `smc` <sub>(ARMv7)</sub> | Hypervisor call / secure monitor call | `r0` holds `0x84000008`, PSCI SYSTEM_OFF. This board's device tree names `hvc`. `smc` is the other conduit. |

## Timer

| Instruction | Name | Effect |
| --- | --- | --- |
| `mrrc` / `mcrr` <sub>(ARMv7)</sub> | Move to two ARM registers from coprocessor / move to coprocessor from two ARM registers | `mrrc p15, 0, c14` reads the 64-bit CNTPCT. `mcrr p15, 2, c14` writes CNTP_CVAL, the compare value. The Hyp fallback uses `mcrr` to clear CNTVOFF. |

The interrupt id is a load from GICC_IAR, and the end of interrupt is a store to GICC_EOIR. Those are ordinary memory accesses. This board enters in Secure SVC and the kernel marks every line Group 1, so it also sets AckCtl. The timer id on this boot is 30. The virtio id is 48.

## Bits named above

| Bit | Name | Meaning |
| --- | --- | --- |
| Mode <sub>(ARMv7)</sub> | CPSR mode field | Bits [4:0]. `0x13` is SVC, `0x1a` is Hyp, `0x10` is user, `0x1f` is System. |
| I | IRQ mask | CPSR bit 7. Set: IRQ stays pending. |
| F <sub>(ARMv7)</sub> | FIQ mask | CPSR bit 6. Set: FIQ stays pending. |
| M | MMU enable | SCTLR bit 0. Set: short-descriptor translation is on. |
| EAE <sub>(ARMv7)</sub> | Extended address enable | TTBCR bit 31. Clear: TTBR0 is a short-descriptor first-level table. |
| PD1 <sub>(ARMv7)</sub> | Translation table walk disable, TTBR1 | TTBCR bit 5. Set: TTBR1 is not consulted. |
| ENABLE | Timer enable | CNTP_CTL bit 0. Set: CNTPCT is compared with CNTP_CVAL. |
| AckCtl <sub>(ARMv7)</sub> | Acknowledge control | GICC_CTLR bit 2. Set: a Secure read of GICC_IAR returns the Group 1 id. |
