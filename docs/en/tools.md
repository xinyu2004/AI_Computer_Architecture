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

`qemu-system-x86` provides `qemu-system-i386`. `labs/i386/qemu/launch.sh` starts that emulator with `Image` as the floppy and `rootfs/hdc-0.11.img` as the first hard disk. A local desktop gets a GTK window. An SSH X11 session gets an SDL window.

`gdb` is the debugger used by the editor. Press F5 and choose **Linux 0.11**. That starts the same machine as `labs/i386/qemu/launch.sh`, pauses the CPU, and stops on the first instruction of `bootsect.s`. Source, single-step, and registers stay in the editor. `labs/i386/qemu/launch.sh` by itself still boots straight to the shell.

When the window does not appear, use VNC instead. The client runs on the computer you are sitting at:

```bash
labs/i386/qemu/launch.sh vnc
```

QEMU listens on `127.0.0.1:5901`. Forward it with `ssh -L 5901:127.0.0.1:5901 <user>@<this-host>`, then open a VNC client on `localhost:5901`.

## Step 3, machine state

`bochs` provides the emulator and its internal debugger. `labs/i386/bochs/launch.sh` starts the same floppy and hard disk as `labs/i386/qemu/launch.sh`. The debugger prompt stays in that terminal. Commands there print registers, the GDT, the IDT, page tables, and the TSS. [What to look at](bochs.md).

`bochs-sdl` provides the SDL window, which is the VGA console when a display is available. `bochsbios` provides `BIOS-bochs-legacy`. `vgabios` provides `vgabios.bin`. With no display, the main `bochs` package already includes the VNC console, on port 5900.

## Step 4, cycles and misses

`python3-dev` provides `Python.h`. gem5 embeds this Python. `python3-venv` provides `python3 -m venv`. `labs/i386/gem5/build.sh` creates `.gem5/.venv` and installs scons there. `m4` is a macro processor the gem5 build runs.

gem5 itself is not an apt package. `labs/i386/gem5/build.sh` clones the pinned release v25.1.0.1 into `.gem5/`, applies `labs/i386/gem5/linux-0.11-x86.patch`, and compiles `.gem5/build/X86/gem5.opt`. That directory stays out of git.

`labs/i386/gem5/launch.sh` loads `src/linux-0.11/tools/system.elf` and runs `setup_paging`. The machine it simulates is described in `labs/i386/gem5/linux-0.11-x86.py`. `labs/i386/gem5/launch.sh timer`, `schedule`, and `copy_page` start at `startup_32` and count the first timer interrupt, the first `_schedule`, and the first inlined `copy_page`. Each prints cycles, L1 misses, and pipeline stalls. [What those runs measure](gem5.md). The build can print a warning that GCC is newer than the range named in the gem5 sources. The binary it produces is the one `launch.sh` runs.

## Step 5, RV32

`gcc-riscv64-unknown-elf` provides `riscv64-unknown-elf-gcc`. The same compiler has a 32-bit multilib. `make -C src/rv32` uses `-march=rv32gc -mabi=ilp32`, so the objects are RV32 even though the command name says riscv64.

`qemu-system-riscv` provides `qemu-system-riscv32`. `labs/rv32/qemu/launch.sh` starts that emulator with `-machine virt -cpu rv32` and leaves the UART at a shell. `help` at that prompt lists the commands. On a Debian or Ubuntu archive that still ships this binary inside `qemu-system-misc`, `script/bootstrap.sh` installs that package instead. [What the RV32 kernel does](rv32.md).

`gdb-multiarch` is the debugger for that kernel. Press F5 and choose **RV32**. It stops at `_start` in `src/rv32/entry.S`, the kernel's first instruction. The UART output is in the terminal of that task. Both debug sessions listen on `127.0.0.1:1234`. Start one at a time. `labs/rv32/qemu/launch.sh` by itself still boots to the shell.
