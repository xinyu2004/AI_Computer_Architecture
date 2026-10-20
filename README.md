

# Learning computer architecture with Linux 0.11

[Chinese](README_zh.md)

This repository is for people learning computer architecture through Linux 0.11. The kernel is small enough to read as a whole. It was written in 1991 for the 80386 and PC devices, and the way through it is to bring that kernel onto contemporary computers, then follow the path below.

## Path

1. **Start from the original.** `reference/linux-0.11/` keeps Linux 0.11 unmodified. Later changes stay comparable with this tree.
2. **Build it on a contemporary computer and run it in QEMU.** [What this step changes, and why](docs/en/build-and-qemu.md). The source is adjusted so today's toolchain can compile it. Change a line, rebuild, and watch what the operating system does: whether it boots, whether a system call returns, and whether a process appears.
3. **Watch the machine state in Bochs.** [What you look at, and how](docs/en/bochs.md). On this same system, follow registers, the GDT, the IDT, page tables, and the TSS as the kernel enters protected mode, takes interrupts, and switches tasks.
4. **Watch the cost of that execution in gem5.** [What you measure, and how](docs/en/gem5.md). Along the same kernel paths, follow cycles, cache misses, and pipeline stalls.
5. **Then look at other architectures.** [RV32 on the virt board](docs/en/rv32.md). After the x86 path works, keep the kernel small and add RISC-V, then ARM, to see what belongs to one architecture and what belongs to the operating system itself. `src/rv32/` is the first of those kernels. `src/rv64/` is the 64-bit register width of that same course. [ARMv7](docs/en/armv7.md) and [AArch64](docs/en/aarch64.md) are the two ARM kernels.

## Course

### [The same questions, on another CPU](docs/en/course.md)

The 80386 answers them first, RV32 answers them again, and ARMv7 and AArch64 answer them in two columns. AArch64 is the 64-bit state of ARMv8. The register pictures are in [mechanism pictures](docs/en/mech.md).

- Which instruction traps into the kernel on a system call
- Where register state is saved across a context switch
- Which register holds the page-table base
- Which register reports the cause of a timer interrupt
- […](docs/en/course.md)

Names that do not say what they do:

- [Instructions in the 80386 kernel](docs/en/i386_instructions.md)
- [Instructions in the RV32 kernel](docs/en/rv32_instructions.md)
- [Instructions in the ARMv7 kernel](docs/en/armv7_instructions.md)
- [Instructions in the AArch64 kernel](docs/en/aarch64_instructions.md)

## What it looks like

- Step through Linux 0.11 with VS Code and QEMU.

  ![Step through Linux 0.11 with VS Code and QEMU](figures/linux_debug.png)

- See what the machine holds during a task switch, in Bochs.

  ![A task switch in Bochs](figures/bochs.png)

- See how many cycles it takes to fill the page tables, in gem5.

  ![Page-table cycles in gem5](figures/gem5.png)

- Split one timer interrupt into a pipeline, in gem5.

  ![Timer interrupt pipeline in gem5](figures/pipe.png)

## Layout


| Path | Role |
| --- | --- |
| `reference/linux-0.11/` | Frozen original kernel |
| `src/linux-0.11/` | 80386 kernel we build and modify |
| `src/rv32/` | RV32 kernel |
| `src/rv64/` | RV64 kernel, same course, 64-bit registers |
| `src/armv7/` | ARMv7 kernel, Cortex-A15 |
| `src/aarch64/` | AArch64 kernel, Cortex-A57 |
| `src/riscv/` | User entry shared by RV32 and RV64 |
| `src/user/` | Programs on the disk: sh, ls, echo, sleep |
| `labs/` | Launch scripts for each step |
| `script/bootstrap.sh` | Install the host tools |
| `rootfs/` | 80386 teaching disk, kept as a download |


The editor hides `reference/` from the file tree and from search. Open that path when comparing it with `src/linux-0.11/`. That tree was unpacked from this kernel.org Historic tarball:

https://cdn.kernel.org/pub/linux/kernel/Historic/old-versions/linux-0.11.tar.gz

SHA256: `660997a6b1f03ee5aadce0ac806ffa653dedf1dea6d82c6731f9c9ddf0092673`

To compare a new download with this tree, check that SHA256, unpack the tarball, and diff the unpacked `linux-0.11` directory against `reference/linux-0.11/`.

`rootfs/hdc-0.11.img` is the teaching disk from [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) `rootfs/hdc-0.11.img.xz`. SHA256: `c83a9837f3f804882a76756f2ff46349467bdf66ed44e301a6c5e124f414b6d1`. Size: 127631360 bytes. It stays a download and is not generated in this tree. The generated `Image` is not kept in git.

## License

`reference/linux-0.11/` is Linus Torvalds' Linux 0.11 (copyright 1991). Those files keep their original terms. The other files in this repository are under the Apache License 2.0; see [LICENSE](LICENSE).