# ARMv7 on the virt board

[中文](../zh/armv7.md)

This is the first ARM kernel of step 5. `src/linux-0.11/` stays the 80386 kernel. `src/armv7/` is a separate kernel for Cortex-A15. It turns a short-descriptor page table on, prints the machine lines, and leaves a shell. The programs on the disk are `ls`, `echo`, and `sleep`. The questions are in [the course](course.md). The stops on this machine are in [the ARMv7 pass](course/armv7.md). AArch64 is the other kernel, in [AArch64](aarch64.md).

Bochs and the gem5 x86 run are unchanged. This kernel is started on its own:

```bash
labs/armv7/qemu/launch.sh
```

The script builds with `arm-none-eabi-gcc -mcpu=cortex-a15`, writes `images/armv7.img`, and boots the raw image `src/armv7/kernel.bin` with `qemu-system-arm -machine virt -cpu cortex-a15`. The console is the PL011. `exit` shuts the machine down. Press F5 and choose **ARMv7** to stop at `_start` in `entry.S`. Linux 0.11, RV32, RV64, ARMv7, and AArch64 listen on `127.0.0.1:1234`. Start one at a time.

## What the run prints

```text
armv7
cpsr 0x600001d3
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$
```

The full `ls`, `echo`, `nosuch`, `sleep`, and `exit` transcript is in [the stops](course/armv7.md). `uart`, `timebase`, and `disk` are the values read from the device tree on this boot. QEMU 10.2.1 produced the numbers above. `timebase` is `CNTFRQ`.

`images/armv7.img` holds ELF32 programs linked at `0x10000`. The kernel loads `sh` and leaves that shell in place. `fork` copies one stack page, `exec` switches to the program you typed, and `exit` switches back.

## The board

`virt` is QEMU's ARM machine. The CPU is `-cpu cortex-a15`. The boot stub places the raw image at `0x40010000`, already in SVC, with the device tree in `r2`. The kernel is linked at that address. `fdt.c` walks the tree. The UART is `arm,pl011`. The interrupt controller is `arm,cortex-a15-gic`, GICv2. The first virtio-mmio device is the disk, and the launch line attaches it to `virtio-mmio-bus.0`. Power-off is PSCI `SYSTEM_OFF`, function id `0x84000008`, through the `hvc` named in the device tree.

## The five jobs

| Job | Where | What changes compared with the 80386 kernel |
| --- | --- | --- |
| Boot | `entry.S`, `main.c` | `_start` is in SVC. There is no boot sector. |
| Page tables | `vm.c` | Short descriptors. `TTBR0` names the table. `TTBCR.EAE` is 0. There is no GDT. |
| Timer | `traps.c` | `CNTP_CVAL` is written before the prompt. The GIC id comes from the device tree and is 30 on this boot. |
| Task switch | `swtch.S` | `exec` saves the shell and enters the child. Both tasks stay in SVC. |
| Copy one page | `mem.c` | `copy_page` copies 1024 32-bit words, 4KiB. |

Instruction names that do not say what they do are on [one page](armv7_instructions.md). The 80386 names are on [the 80386 page](i386_instructions.md). The RV32 names are on [the RV32 page](rv32_instructions.md).
