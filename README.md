

# Learning computer architecture with Linux 0.11

[Chinese](README_zh.md)

This repository is for people learning computer architecture through Linux 0.11. The kernel is small enough to read as a whole. It was written in 1991 for the 80386 and PC devices, and the way through it is to bring that kernel onto contemporary computers, then look further along the steps below.

Before step 2, run `script/bootstrap.sh` once. [What it installs, and why](docs/en/tools.md). Instruction names that do not say what they do, such as `lmsw`, are on one page: [Instructions in the 80386 kernel](docs/en/i386_instructions.md).

## Steps

1. **Start from the original.** `reference/linux-0.11/` keeps Linux 0.11 unmodified. Later changes stay comparable with this tree.
2. **Build it on a contemporary computer and run it in QEMU.** [What this step changes, and why](docs/en/build-and-qemu.md). The source is adjusted so today's toolchain can compile it. Change a line, rebuild, and watch what the operating system does: whether it boots, whether a system call returns, and whether a process appears.
3. **Watch the machine state in Bochs.** [What you look at, and how](docs/en/bochs.md). On this same system, follow registers, the GDT, the IDT, page tables, and the TSS as the kernel enters protected mode, takes interrupts, and switches tasks.
4. **Watch the cost of that execution in gem5.** [What you measure, and how](docs/en/gem5.md). [Walk through the chapters](docs/en/cases.md). Along the same kernel paths, follow cycles, cache misses, and pipeline stalls.
5. **Then look at other architectures.** [RV32 on the virt board](docs/en/rv32.md). After the x86 path works, keep the kernel small and add RISC-V, then ARM, to see what belongs to one architecture and what belongs to the operating system itself. `src/rv32/` is the first of those kernels.

Work that is not on this path is listed in [the backlog](docs/en/backlog.md). Generating `rootfs/hdc-0.11.img` is there. It stays off the numbered steps.

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
| `src/linux-0.11/`                       | 80386 kernel we build and modify       |
| `src/rv32/`                             | RV32 kernel, beside the 80386 one      |
| `rootfs/`                               | Root filesystem image                  |
| `images/`                               | Generated boot image (not kept in git) |
| `script/bootstrap.sh`                   | Install the host tools for the current step |
| `labs/i386/qemu/` `labs/i386/bochs/` `labs/i386/gem5/` | 80386 experiments                           |
| `labs/rv32/qemu/`                            | QEMU virt launcher for the RV32 kernel      |


The editor hides `reference/` from the file tree and from search. Open that path when comparing it with `src/linux-0.11/`.

`src/linux-0.11/` builds on a contemporary host. `labs/i386/qemu/launch.sh` boots it in QEMU, and the shell can run `ls`. Press F5 and choose **Linux 0.11** to stop on the first instruction and step in the editor. `labs/i386/bochs/launch.sh` opens the same disks in Bochs; the debugger in that terminal shows the GDT, the IDT, page tables, and the TSS. `labs/i386/gem5/launch.sh` runs `setup_paging` on a gem5 out-of-order CPU and prints cycles, L1 misses, and a store-queue stall. `labs/i386/gem5/launch.sh timer` continues into the kernel and counts the first `_timer_interrupt`; with no root disk that interrupt is taken from the `panic("Unable to mount root")` spin. `labs/i386/gem5/launch.sh schedule` counts the first `_schedule`, which switches from task 0 to task 1. `labs/i386/gem5/launch.sh copy_page` counts the first 4KiB `rep movsl` inlined in `_un_wp_page`. `labs/rv32/qemu/launch.sh` builds the RV32 kernel and boots it to a shell on QEMU's virt board; `help` lists the commands, and the shell can run `ls`. Press F5 and choose **RV32** to stop at `_start` in `entry.S`. **Linux 0.11** in that menu is still the 80386 session. Both listen on `127.0.0.1:1234`; start one at a time.

`rootfs/hdc-0.11.img` is the teaching disk from [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) `rootfs/hdc-0.11.img.xz`. SHA256: `c83a9837f3f804882a76756f2ff46349467bdf66ed44e301a6c5e124f414b6d1`. Size: 127631360 bytes. The generated `Image` is not kept in git.

## License

`reference/linux-0.11/` is Linus Torvalds' Linux 0.11 (copyright 1991). Those files keep their original terms. The other files in this repository are under the Apache License 2.0; see [LICENSE](LICENSE).