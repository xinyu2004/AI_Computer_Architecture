# Stops on ARMv7

[中文](../../zh/course/armv7.md)

[Question table](../course.md)

Boot with `labs/armv7/qemu/launch.sh`. The UART is in that terminal. Press F5 and choose **ARMv7** to stop at `_start` in `entry.S`, then set the next breakpoint. Break on the function. Addresses belong to this build.

## Read this on the UART first

```text
armv7
cpsr 0x600001d3
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

`sleep 10` waits 10ms, which looks like no wait on the terminal. Use `sleep 1000` to see the pause. `uart`, `timebase`, and `disk` come from this boot's device tree. QEMU 10.2.1 printed the numbers above. `timebase` is the value read from `CNTFRQ`. `exit` makes QEMU quit.

`fork` in `exec.c` calls `copy_page`. After `exec` loads a program it calls `swtch`, and the child starts at its own `_start`. `exit` switches back to the shell. The pages saved at `fork` are the ones restored. The kernel does not read `sh` from the disk again.

## Where to stop

### Reset {#boot}

F5 stops at `_start`. QEMU's boot stub has already left the CPU in SVC and jumps here. `r2` holds the device tree. Continue, and the UART prints `cpsr 0x600001d3`. The low 5 bits are `0x13`, which is SVC.

### Stay in SVC {#mode}

This boot does not change mode. Break on the `cpsr` read in `main.c`. The low 5 bits of the printed value are still `0x13`.

### TTBR0 {#pages}

Break in `vm_on` in `vm.c`, on the write to `TTBR0`. `TTBCR.EAE` stays 0, so the table is a short-descriptor table of 4KiB small pages. Step past it and the UART prints `ttbr0 on`.

### User mode {#privilege}

After `$` appears, break in `syscall` in `exec.c`. Press Enter. The low 5 bits of `spsr` are `0x10`, user mode. The call number is in `r7`.

### Timer {#timer}

`$` is up and nothing has been typed. Break in `irq_handler` in `traps.c`. `gic_iar` returns 30, the GIC id of the non-secure physical timer from the device tree. `current_name` is `sh`. The handler writes `CNTP_CVAL`. One tick is `CNTFRQ / 100`.

This board enters in Secure SVC. `gic_enable` marks the line Group 1. The CPU interface forwards that line only with Group 1 enabled, and a Secure read of `GICC_IAR` returns the id only when AckCtl is set. Without AckCtl the read is 1022 and the interrupt stays pending.

### svc {#ecall}

Break at `svc_entry` in `trap.S`. Type `echo hello`. The shell's `exec` arrives with `r7` equal to 3. `echo` writing the UART arrives with `r7` equal to 1. Hardware leaves the return address in `lr`, already on the instruction after `svc`. The UART prints `hello`.

### swtch {#switch}

Break at `swtch` in `swtch.S`. Type `echo hello` again. The first stop saves the shell and loads `echo`'s `_start`. The saved registers are `r4` through `r11`, plus `lr` and `sp`. `echo` exiting enters once more and returns to the shell's next read.

### copy_page {#copy}

Break at `copy_page` in `mem.c`. Type `ls` again. The function runs once. The source is the shell's stack page and the destination is the child's, 1024 words, 4096 bytes.

Type `nosuch`. The UART prints `not found`. Neither `swtch` nor `copy_page` is entered.

### One byte {#byte}

After `$` appears, break at `uart_getc` in `uart.c`. Type a letter. The flag register is at offset `0x18`. Bit 4 set means the receive FIFO is empty. The data register is at offset 0. Receive is not an interrupt: the timer wakes the wait, then the flag is read again.

### One line {#line}

Break at `read_line` in `uart.c`. Type `echo hello`. The letters are collected here, and Enter hands the whole line to the shell.

### A name {#name}

Break at `lookup` in `fs.c`. Type `echo hello` again. The argument is `echo`. On return, `name_block` is that file's sector.

### A sector {#block}

Break at `bread` in `fs.c`. The first `echo hello` enters `virtio_read`. The same command again returns the cached block and does not enter `virtio_read`.

### Disk done {#disk}

Break in `irq_handler` and type a command that has not been loaded yet, such as the first `echo hello`. `gic_iar` returns 48, the GIC id of the first virtio device in the device tree. `virtio_ack` writes the virtio interrupt-acknowledge register, and `gic_eoi` completes the interrupt. The next `echo hello` does not enter here. That block is still in the `bread` cache.
