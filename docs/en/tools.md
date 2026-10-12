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

`qemu-system-x86` provides `qemu-system-i386`. `labs/qemu/launch.sh` starts that emulator with `Image` as the floppy and `rootfs/hdc-0.11.img` as the first hard disk. The console is the QEMU window.

## Later steps

Bochs is the step 3 tool, for registers, the GDT, the IDT, page tables, and the TSS. gem5 is the step 4 tool, for cycles and cache misses. `script/bootstrap.sh` does not install them yet.
