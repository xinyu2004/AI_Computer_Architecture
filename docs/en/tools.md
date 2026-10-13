# Host tools

[中文](../zh/tools.md)

Run this once, from the repository root, before building or booting:

```bash
script/bootstrap.sh
```

The script installs the tools for the steps that are ready, and it says which step each package serves. On Debian and Ubuntu it runs one `apt-get install`. On any other system it prints the same list and stops, so you can install those commands with your own packages.

## Step 2, build the kernel

`build-essential` provides `make`, `as`, and `ld`. `make -C src/linux-0.11` uses them to turn the kernel into `src/linux-0.11/Image`.

`gcc-multilib` provides `gcc -m32`. The host gcc defaults to 64-bit objects. This kernel is 32-bit 80386 code, so the compiler has to be able to emit that.

## Step 2, run it

`qemu-system-x86` provides `qemu-system-i386`. `labs/qemu/launch.sh` starts that emulator with `Image` as the floppy and `rootfs/hdc-0.11.img` as the first hard disk. A local desktop gets a GTK window. An SSH X11 session gets an SDL window.

`gdb` is the debugger used by the editor. Press F5 and choose **Linux 0.11**. That starts the same machine as `labs/qemu/launch.sh`, pauses the CPU, and stops on the first instruction of `bootsect.s`. Source, single-step, and registers stay in the editor. `labs/qemu/launch.sh` by itself still boots straight to the shell.

When the window does not appear, use VNC instead. The client runs on the computer you are sitting at:

```bash
labs/qemu/launch.sh vnc
```

QEMU listens on `127.0.0.1:5901`. Forward it with `ssh -L 5901:127.0.0.1:5901 <user>@<this-host>`, then open a VNC client on `localhost:5901`.

## Step 3, machine state

`bochs` provides the emulator and its internal debugger. `labs/bochs/launch.sh` starts the same floppy and hard disk as `labs/qemu/launch.sh`. The debugger prompt stays in that terminal. Commands there print registers, the GDT, the IDT, page tables, and the TSS. [What to look at](bochs.md).

`bochs-sdl` provides the SDL window, which is the VGA console when a display is available. `bochsbios` provides `BIOS-bochs-legacy`. `vgabios` provides `vgabios.bin`. With no display, the main `bochs` package already includes the VNC console, on port 5900.

## Later steps

gem5 is the step 4 tool, for cycles and cache misses. `script/bootstrap.sh` does not install it yet.
