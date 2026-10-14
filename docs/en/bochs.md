# Watch the machine state in Bochs

[中文](../zh/bochs.md)

This is learning step 3. The disks are the same ones QEMU boots: `src/linux-0.11/Image` and `rootfs/hdc-0.11.img`. Bochs adds a debugger that prints registers, the GDT, the IDT, page tables, and the TSS.

`src/linux-0.11/` is not modified for this step. Breakpoints are set from the debugger. The kernel does not contain a magic breakpoint instruction.

Pressing F5 still debugs the QEMU machine and stops on the first instruction of `bootsect.s`. That session is for source and single-step. This step is for the descriptor tables and the task state, which that session does not show.

## The machine

`labs/i386/bochs/bochsrc` is a 16MB PC. The floppy is `Image`. The hard disk is `rootfs/hdc-0.11.img`, 410 cylinders, 16 heads, 38 sectors, which is the size of that file. There is no network device.

The CPU model is `pentium`. That is the oldest named model in Bochs 2.8, and it still runs the 386 protected-mode instructions this kernel uses. `ips=1000000` makes the floppy's emulated delays finish after fewer instructions, so the boot reaches the debugger in a few seconds. The BIOS is `/usr/share/bochs/BIOS-bochs-legacy`.

## How to check

From the repository root, after `script/bootstrap.sh`:

```bash
make -C src/linux-0.11
labs/i386/bochs/launch.sh
```

The script prints the breakpoints for this build, then Bochs takes the terminal. The debugger prompt looks like this:

```text
Bochs internal debugger, type 'help' for help or 'c' to continue
```

A local display opens an SDL window. That window is the VGA console. Type the commands in this terminal. With no display, or with an explicit request, the console is VNC:

```bash
labs/i386/bochs/launch.sh vnc
```

Bochs prints the port it opened, usually `5900`. The debugger prompt appears after a VNC client connects. From the computer you are sitting at:

```bash
ssh -L 5900:127.0.0.1:5900 <user>@<this-host>
```

Then open a VNC client on `localhost:5900`.

`q` leaves the debugger. `help` lists commands. If Bochs reports that the disk image is locked, another emulator already has `rootfs/hdc-0.11.img` open. Close that window and start this script again.

The addresses below are an example from one link. `labs/i386/bochs/launch.sh` prints the addresses for the tree you just built. The kernel addresses change every time you link. The `lmsw` address changes only when `boot/setup.s` changes. Setup is loaded at physical `0x90200`, and the script adds the file offset of `lmsw` from `boot/setup.elf`.

## Protected mode

In `boot/setup.s`, `lmsw` sets CR0.PE and the next instruction is `ljmp $8,$0`. On the example build, `lmsw` is at linear `0x90301`. Use the address the script prints:

```text
pb 0x90301
c
```

The CPU is on `lmsw`. PE is still clear. The first `s` lands on `ljmp $8,$0`. The second `s` takes that jump:

```text
s
s
creg
sreg
info gdt 0 2
```

`creg` shows `PE`. `sreg` shows CS as selector `0x0008`: a 32-bit code segment, base `0`, limit `0x7fffff`. `info gdt 0 2` shows the table `lgdt` loaded. Entry 0 is the null descriptor. Selector 8 is that code segment. Selector `0x10` is the data segment, also base 0 and limit `0x7fffff`. GDTR's base is the GDT inside setup (`0x90314` when `lmsw` is at `0x90301`). The limit `0x800` is the temporary table from `setup.s`. `info gdt` without a range prints every slot up to that limit.

## Paging

`setup_paging` in `boot/head.s` writes the page directory at physical 0 and then sets CR0.PG. The script prints its address (`0x5450` on the example link):

```text
pb 0x5450
c
creg
```

The first instruction is `movl $1024*5,%ecx`. PG is still clear. Step until `movl %eax,%cr0` has executed, then run `creg` again. PG is set and CR3 is 0, the page directory. `page 0` translates linear address 0. The directory identity-maps the first 16MB, four page tables of 4MB each.

## Timer interrupt

`setup.s` programs the first 8259 so IRQ0 arrives as vector `0x20`. The handler is `_timer_interrupt` in `kernel/system_call.s`. The script prints its address (`0x6ee4` on the example link):

```text
pb 0x6ee4
c
info idt 0x20
sreg
```

`info idt 0x20` shows a 32-bit interrupt gate, DPL 0, whose target is that handler. The stack holds the address the interrupt will return to.

## Task switch

`_schedule` decides which task runs. The switch is the `ljmp` in the `switch_to` macro in `include/linux/sched.h`. The script prints the address of `_schedule` (`0x6814` on the example link):

```text
pb 0x6814
c
info tss
sreg
```

`info tss` prints the task state segment. After the far jump in `switch_to`, `sreg` shows TR pointing at the TSS of the task that is now current.
