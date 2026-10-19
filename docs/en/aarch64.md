# AArch64 on the virt board

[中文](../zh/aarch64.md)

This is the second ARM kernel of step 5. `src/aarch64/` is the Cortex-A57 kernel and does not share its implementation with `src/armv7/`. It turns a 4KiB page table on, prints the machine lines, and leaves the same shell. The stops are in [the AArch64 pass](course/aarch64.md). ARMv7 is in [ARMv7](armv7.md).

```bash
labs/aarch64/qemu/launch.sh
```

The script builds with `aarch64-linux-gnu-gcc -mcpu=cortex-a57`, writes `images/aarch64.img`, and boots the raw image `src/aarch64/kernel.bin` with `qemu-system-aarch64 -machine virt -cpu cortex-a57`. Press F5 and choose **AArch64** to stop at `_start`. The five debug sessions listen on `127.0.0.1:1234`. Start one at a time.

## What the run prints

```text
aarch64
currentel 0x00000004
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$
```

`currentel` value `0x4` is EL1. `timebase` is `CNTFRQ_EL0`. QEMU 10.2.1 produced these numbers on this boot. The user programs are ELF64, linked at `0x10000`, with 64-bit pointers.

## The board

The boot stub places the image at `0x40080000`, already in EL1h, with the device tree in `x0`. The kernel is linked there. The UART, GICv2, and virtio addresses still come from the device tree. The interrupt controller's `compatible` string on this tree is `arm,cortex-a15-gic`. Power-off is still PSCI `0x84000008` through `hvc`.

## The five jobs

| Job | Where | What changes compared with the ARMv7 kernel |
| --- | --- | --- |
| Boot | `entry.S`, `main.c` | `_start` is at EL1. The vector table is `VBAR_EL1`. |
| Page tables | `vm.c` | `TTBR0_EL1`, 4KiB granule, `T0SZ` 25. |
| Timer | `traps.c` | `CNTP_CVAL_EL0`. The GIC id is still 30 on this boot. |
| System call | `trap.S` | `svc` comes from EL0. The EC field of `ESR_EL1` is `0x15`, and the call number is in `x8`. |
| Copy one page | `mem.c` | `copy_page` copies 512 64-bit words, still 4KiB. |

Instruction names that do not say what they do are on [one page](aarch64_instructions.md). The ARMv7 names are on [the ARMv7 page](armv7_instructions.md). The 80386 names are on [the 80386 page](i386_instructions.md).
