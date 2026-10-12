

# Learning computer architecture with Linux 0.11

[Chinese](README_zh.md)

This repository is for people learning computer architecture through Linux 0.11. The kernel is small enough to read as a whole. It was written in 1991 for the 80386 and PC devices, and the way through it is to bring that kernel onto contemporary computers, then look further along the steps below.

## Steps

1. **Start from the original.** `reference/linux-0.11/` keeps Linux 0.11 unmodified. Later changes stay comparable with this tree.
2. **Build it on a contemporary computer and run it in QEMU.** The source is adjusted so today's toolchain can compile it. Change a line, rebuild, and watch what the operating system does: whether it boots, whether a system call returns, and whether a process appears.
3. **Watch the machine state in Bochs.** On this same system, follow registers, the GDT, the IDT, page tables, and the TSS as the kernel enters protected mode, takes interrupts, and switches tasks.
4. **Watch the cost of that execution in gem5.** Along the same kernel paths, follow cycles, cache misses, and pipeline stalls.
5. **Then look at other architectures.** After the x86 path works, keep the kernel small and add RISC-V, then ARM, to see what belongs to one architecture and what belongs to the operating system itself.



## What you come away with

Along this path, the source leads to what the operating system does, that behavior leads to the machine state behind it, and that state leads to what the execution costs. A concrete architecture and the operating system itself become visible side by side.

## Original source

`reference/linux-0.11/` was unpacked from this kernel.org Historic tarball:

https://cdn.kernel.org/pub/linux/kernel/Historic/old-versions/linux-0.11.tar.gz

SHA256: `660997a6b1f03ee5aadce0ac806ffa653dedf1dea6d82c6731f9c9ddf0092673`

To compare a new download with this tree, check that SHA256, unpack the tarball, and diff the unpacked `linux-0.11` directory against `reference/linux-0.11/`.

## Layout


| Path                                    | Role                                   |
| --------------------------------------- | -------------------------------------- |
| `reference/linux-0.11/`                 | Frozen original kernel                 |
| `src/linux-0.11/`                       | Kernel we build and modify             |
| `rootfs/`                               | Root filesystem image                  |
| `images/`                               | Generated boot image (not kept in git) |
| `labs/qemu/` `labs/bochs/` `labs/gem5/` | Experiments for each environment       |


This version contains the frozen kernel only. The buildable tree, root filesystem, and labs come next.

## License

`reference/linux-0.11/` is Linus Torvalds' Linux 0.11 (copyright 1991). Those files keep their original terms. The other files in this repository are under the Apache License 2.0; see [LICENSE](LICENSE).