# Stops on AArch64

[中文](../../zh/course/aarch64.md)

[Question table](../course.md)

Boot with `labs/aarch64/qemu/launch.sh`. The UART is in that terminal. Press F5 and choose **AArch64** to stop at `_start` in `entry.S`, then set the next breakpoint. Break on the function. Addresses belong to this build.

This is a separate kernel, not the ARMv7 sources behind a conditional. The exception level, the page tables, and the system registers are not shared.

## Read this on the UART first

```text
aarch64
currentel 0x00000004
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$ ls
ls
sh
echo
sleep
$ echo hello
hello
$ echo hello
hello
$ nosuch
not found
$ sleep 10
$ exit
```

`sleep 10` waits 10ms. `uart`, `timebase`, and `disk` come from this boot's device tree. QEMU 10.2.1 printed the numbers above. `timebase` is `CNTFRQ_EL0`. The `currentel` value `0x4` is EL1. `exit` makes QEMU quit.

The programs on the disk are still the four C files in `src/user/`. The syscall wrappers are in `src/aarch64/user.h`: `svc #0`, the number in `x8`, the arguments in `x0` through `x2`. Pointers are 64 bits and the stack is aligned to 16 bytes.

## Where to stop

### Reset {#boot}

F5 stops at `_start`. The image is linked at `0x40080000`. QEMU's boot stub jumps there with the device tree in `x0`. Continue, and the UART prints `currentel 0x00000004`, which is EL1.

### Stay at EL1 {#mode}

This boot is already in EL1h. Break on the `CurrentEL` read in `main.c`. The value is `0x4`.

### TTBR0_EL1 {#pages}

Break in `vm_on` in `vm.c`, on the write to `TTBR0_EL1`. `T0SZ` is 25, so a virtual address is 39 bits. The granule is 4KiB and the walk starts at level 1. Step past it and the UART prints `ttbr0 on`.

### EL0 {#privilege}

After `$` appears, break in `syscall` in `exec.c`. Press Enter. `SPSR_EL1` is 0, which is EL0t. The call number is in `x8`.

### Timer {#timer}

`$` is up and nothing has been typed. Break in `irq_handler` in `traps.c`. `gic_iar` returns 30. From user mode this entry is the slot at `VBAR_EL1` plus `0x480`. A wait inside the kernel uses the slot at offset `0x280`. The handler writes `CNTP_CVAL_EL0`.

The interrupt controller in the device tree is GICv2, `compatible` `arm,cortex-a15-gic`. The id is still read from `GICC_IAR`.

### svc {#ecall}

Break at `svc_entry` in `trap.S`. Type `echo hello`. This is the slot at `VBAR_EL1` plus `0x400`, a synchronous exception from EL0 in AArch64. The EC field of `ESR_EL1` is `0x15`. The shell's `exec` arrives with `x8` equal to 3. `ELR_EL1` already points at the instruction after `svc`. The UART prints `hello`.

### swtch {#switch}

Break at `swtch` in `swtch.S`. Type `echo hello` again. The saved registers are `x19` through `x30`, plus `sp`. `echo` exiting enters once more and returns to the shell's next read.

### copy_page {#copy}

Break at `copy_page` in `mem.c`. Type `ls` again. The function runs once: 512 64-bit words, still 4096 bytes.

Type `nosuch`. The UART prints `not found`. Neither `swtch` nor `copy_page` is entered.

### One byte {#byte}

After `$` appears, break at `uart_getc` in `uart.c`. Type a letter. This is still the PL011. Bit 4 of the flag register at offset `0x18` means the receive FIFO is empty, and the data register is at offset 0. The timer wakes the wait.

### One line {#line}

Break at `read_line` in `uart.c`. Type `echo hello`. Enter hands the whole line to the shell.

### A name {#name}

Break at `lookup` in `fs.c`. Type `echo hello` again. The argument is `echo`. On return, `name_block` is that file's sector.

### A sector {#block}

Break at `bread` in `fs.c`. The first `echo hello` enters `virtio_read`. The same command again returns the cached block.

### Disk done {#disk}

Break in `irq_handler` and type a command that has not been loaded yet. `gic_iar` returns 48, the same GIC id this ARMv7 boot read. `virtio_ack` writes the acknowledge, and `gic_eoi` completes the interrupt. The next `echo hello` does not enter here.
