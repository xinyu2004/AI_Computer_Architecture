# Build it on a contemporary computer and run it in QEMU

[中文](../zh/build-and-qemu.md)

This is learning step 2. After it, the kernel can be built with a contemporary toolchain and booted in QEMU. A one-line change can be recompiled, and the running system shows whether it boots, whether a system call returns, and whether a process appears.

`reference/linux-0.11/` stays the unmodified 1991 tree. The buildable kernel lives in `src/linux-0.11/`. A diff between those two trees is the whole of this step.

The method follows ports that already boot Linux 0.11 on current hosts, in particular [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11). Those trees show a route that works. This repository does not replace `src/linux-0.11/` with one of them. Scheduling, memory management, and system calls stay as they are in the original.

## Why the 1991 tree does not build

`reference/linux-0.11/Makefile` still names the 1991 tools and the 1991 object format.

The host is a 64-bit computer. This kernel is 32-bit 80386 code. A plain `gcc` on the host produces 64-bit objects, and the old flags `-fcombine-regs` and `-mstring-insns` are gone from current GCC.

`boot/bootsect.s` and `boot/setup.s` are as86 assembly, built with `as86` and `ld86`. Current binutils provides GNU `as`. `boot/head.s` and the rest of the kernel assembly are already GNU as, so the boot files are the part that does not match the assembler students have.

`tools/build.c` packs three pieces into the floppy image `Image`: a boot sector, setup, and the kernel. It expects a.out headers (`MINIX_HEADER` of 32 bytes, or the old `GCC_HEADER` of 1024 bytes). Current GCC writes ELF, so those header sizes no longer describe the files.

The kernel sources also contain no shell. Booting `Image` alone does not reach a login. A root filesystem image has to be attached, as `ROOT_DEV` in the original Makefile already assumes (`/dev/hd6`).

## What this step changes

### Toolchain

The Makefile uses `gcc -m32`, `as --32`, and `ld -m elf_i386`, and drops the flags current GCC rejects. `-std=gnu89 -fno-stack-protector -fno-builtin -nostdinc -fno-pic` keeps the dialect and the freestanding build the 1991 sources were written for. Stack protectors and position-independent code would insert calls and addressing the original kernel does not have.

### Boot assembly

`bootsect.s` and `setup.s` are rewritten from as86 into GNU as. The load addresses are unchanged.

as86 wrote `mov ax,#BOOTSEG` and `jmpi go,INITSEG`. GNU as does not accept that syntax, and on a 64-bit host it assembles 32-bit instructions unless the file starts with `.code16`. The BIOS enters the floppy boot sector in 16-bit real mode, so the new text uses `.code16`, `movw $BOOTSEG, %ax`, and `ljmp $INITSEG, $go`. The operand order of `ljmp` is the reverse of as86's `jmpi`. The jump is still `0x9000:go`.

The addresses stay because the rest of boot still looks for them. The BIOS places the boot sector at `0x7C00`. The sector copies itself to `0x90000`, setup to `0x90200`, and the kernel to `0x10000`. `setup.s` stores the BIOS parameters at `0x90000`, and `head.s` reads them from there. `head.s` was already GNU as and stays on the same protected-mode path. The as86 text remains in `reference/` for comparison.

### Floppy image

`objcopy -O binary` strips the ELF headers. `tools/build.c` then writes the same `Image` layout as before: a 512-byte boot sector, the setup sectors, and the kernel, including the original root-device number. The image format is what QEMU boots. The ELF headers are an artifact of the contemporary linker, not part of the 1991 boot protocol.

### C that current GCC rejects

Fixes are limited to constructs current GCC will not compile, such as the old `extern inline` and implicit function declarations. Each of those edits exists so the file compiles. Process scheduling, paging, and the system-call path are not part of this step.

### Root filesystem and QEMU

The root image is the teaching disk shipped as `rootfs/hdc-0.11.img.xz` in [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11), unpacked to `rootfs/hdc-0.11.img`.

SHA256: `c83a9837f3f804882a76756f2ff46349467bdf66ed44e301a6c5e124f414b6d1`

Size: 127631360 bytes.

QEMU is started with 16MB of RAM, `Image` as the boot floppy, and that file as the first hard disk. Sixteen megabytes matches the memory size this kernel and that root image are used with. The keyboard in `include/linux/config.h` is `KBD_US`, the same switch that port makes, because QEMU sends a US keyboard.

### Inline assembly that current GCC miscompiles

Several 1991 `asm` statements name a register as an input and then change it (`rep movs`, `lodsl`, `bts`). GCC 15 keeps that register for the surrounding C. The hello-os tree avoids this by compiling without `-O`. This tree stays at `-O2` and marks the changed registers as outputs, which is the same correction. The two that decide whether a shell appears are `COPYBLK` in `fs/buffer.c` (a page-fault read of `/bin/sh` was landing 2048 bytes past the page and overwriting the next task) and `find_first_zero` in `fs/bitmap.c` (`lodsl` left `esi` pointing at the next bitmap word, so `new_block` panicked with "bit already set").

## How to check

From the repository root, install the host tools once with `script/bootstrap.sh`. [What it installs, and why](tools.md). Then build the floppy image.

```bash
make -C src/linux-0.11
```

`tools/build` prints `Root device is (3, 1)`. The result is `src/linux-0.11/Image`.

Boot that image as the floppy and `rootfs/hdc-0.11.img` as the first hard disk:

```bash
labs/i386/qemu/launch.sh
```

The kernel writes the console to VGA memory. `-nographic` stays blank. On a local desktop this opens a GTK window. Over SSH X11 forwarding it opens an SDL window: forwarded displays are numbered 10 and up (`localhost:10.0`), which is how the script tells them from a local `:0`. That path also turns OpenGL off, so an X server such as XLaunch does not hit the GTK monitor warning.

When that window does not appear, start the same machine over VNC:

```bash
labs/i386/qemu/launch.sh vnc
```

QEMU then listens on `127.0.0.1:5901`. From the computer you are sitting at, forward that port and open a VNC client on `localhost:5901`:

```bash
ssh -L 5901:127.0.0.1:5901 <user>@<this-host>
```

The same script accepts `gtk` or `sdl` when you want to override the automatic choice. With no `DISPLAY` and no Wayland display, `labs/i386/qemu/launch.sh` already selects VNC.

The drives are always:

```bash
qemu-system-i386 -m 16M -boot a -nic none \
  -drive file=src/linux-0.11/Image,if=floppy,format=raw \
  -drive file=rootfs/hdc-0.11.img,if=ide,format=raw,index=0
```

The console reaches `[/usr/root]#`. Typing `ls /` lists `bin`, `dev`, `etc`, `shoelace`, `usr`, and `var`.

## Debug

Running `labs/i386/qemu/launch.sh` and debugging are separate. The script boots to the shell. Single-step and registers stay in the editor, because a paused QEMU window has neither.

Press F5 and choose **Linux 0.11**. `.vscode/tasks.json` starts the same `launch.sh` in the background and pauses the CPU. `.vscode/launch.json` connects GDB to `127.0.0.1:1234`, loads the symbols, and stops on the first instruction of this system:

```
bootsect.s: movw $BOOTSEG, %ax
```

That instruction is at `0x7C00`. The breakpoint is a hardware breakpoint, so it is still there after the BIOS loads the boot sector. Stepping works once the session is paused: the toolbar shows continue, rather than pause. In the watch view the program counter is `$pc`, not `pc`. The QEMU window uses the same display choice as `launch.sh`: GTK on a local desktop, SDL over SSH X11, VNC when there is no display. Source, stepping, and registers are in the editor.

`boot/bootsect.elf` matches the instructions that run at `0x7C00`. Single-step from that first `movw` covers the copy loop. The following `ljmp` to `0x9000` changes the segment. GDB then reports only the offset inside the segment, so the later lines of `bootsect.s` do not line up.

`boot/setup.elf` is placed at `0x90200` for `setup.s`. `tools/system.elf` matches `head.s` and the C sources at physical address 0, where protected mode begins. C names keep the leading underscore, so the kernel entry in C is `_main`. That file stays unloaded at this stop: its address range covers `0x7C00` and would hide the boot sector.

`-g` stays in those ELF files. The floppy `Image` is still the raw binary, and the compiler is still `-O2`, so later C lines can be skipped. BIOS has no symbols.

If F5 reports that the disk is locked, a QEMU window is already open on `rootfs/hdc-0.11.img`. Close it and start the debug session again. `labs/i386/qemu/launch.sh` on its own does not pause and does not open the GDB port.

`diff -rq reference/linux-0.11 src/linux-0.11` shows the toolchain, boot-syntax, image-packing, and compiler fixes above. Scheduling, paging, and the system calls themselves are still the 1991 code. Bochs and gem5 are the next steps.
