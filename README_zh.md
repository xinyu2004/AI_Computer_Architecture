# 用 Linux 0.11 学习计算机体系结构

[English](README.md)

这个仓库面向想用 Linux 0.11 学习计算机体系结构的人。这份内核小到可以整份读完，写于 1991 年，面向 80386 和 PC 设备。学习时先把它接到当代计算机上，再顺着下面的顺序看下去。

第 2 步之前，先运行一次 `script/bootstrap.sh`。[它安装什么，以及为什么](docs/zh/tools.md)。名字本身看不出作用的指令，例如 `lmsw`，集中在一页里：[80386 内核里的指令](docs/zh/i386_instructions.md)。

## 步骤

1. **从原版读起。** `reference/linux-0.11/` 保存未经修改的 Linux 0.11。后面的改动都回到这里对照。
2. **在当代计算机上编译，并在 QEMU 里运行。** [这一步改了什么，以及为什么](docs/zh/build-and-qemu.md)。源码改到能用现在的工具链编译。改一行、重新编译，就可以观察操作系统在做什么：能否启动、系统调用是否返回、进程是否出现。
3. **在 Bochs 里看机器状态。** [看什么，以及怎么看](docs/zh/bochs.md)。同一个系统进入保护模式、响应中断、切换任务时，观察寄存器、GDT、IDT、页表和 TSS 的变化。
4. **在 gem5 里看执行代价。** [量什么，以及怎么量](docs/zh/gem5.md)。[按章节走一遍](docs/zh/cases.md)。沿同一段内核路径，观察周期、cache miss 和流水线停顿。
5. **再看别的架构。** [virt 板上的 RV32](docs/zh/rv32.md)。x86 走通之后，保持内核短小，先加入 RISC-V，再加入 ARM，对照哪些属于某一种架构，哪些是操作系统本身。`src/rv32/` 是其中的第一份内核。

不在这条路径上的工作记在 [backlog](docs/zh/backlog.md)。生成 `rootfs/hdc-0.11.img` 在那里，不进入上面的编号步骤。

## 看起来是什么样

按 F5 并选择 **Linux 0.11**，会停在 `boot/bootsect.s` 的 `start`。EIP 是 `0x7c00`，还在实模式，引导扇区还没被拷走。

![停在引导扇区的第一条指令](figures/linux_debug.png)

Bochs 停在 `_schedule` 的 `push %esi`。`info tss` 是任务状态，`sreg` 是段，客户 shell 已经执行过 `echo hello`。

![Bochs 停在 schedule，旁边是任务状态和客户 shell](figures/bochs.png)

`labs/i386/gem5/launch.sh` 统计 `setup_paging`。这一次是 36047 个周期，`stosl` 循环造成 23832 次 store-queue 停顿。

![setup_paging 的 gem5 计数](figures/gem5.png)

`labs/i386/gem5/launch.sh timer` 写出 `labs/i386/gem5/m5out/pipeline.html`。每一行是 `_timer_interrupt` 的一条微操作。左端是取指。长条是写 DS。

![时钟中断的流水线](figures/pipe.png)

## 收获

沿着这条路径，可以从源码看到操作系统的行为，从行为看到机器状态，从状态看到执行代价，并分得清一种具体架构和操作系统本身。

## 原版出处

`reference/linux-0.11/` 从 kernel.org Historic 的这个压缩包解出：

https://cdn.kernel.org/pub/linux/kernel/Historic/old-versions/linux-0.11.tar.gz

SHA256：`660997a6b1f03ee5aadce0ac806ffa653dedf1dea6d82c6731f9c9ddf0092673`

要和这份树比较，先核对 SHA256，再解压，把解出的 `linux-0.11` 目录与 `reference/linux-0.11/` 做 diff。

## 目录

| 路径 | 作用 |
| --- | --- |
| `reference/linux-0.11/` | 冻结的原始内核 |
| `src/linux-0.11/` | 要编译、要修改的 80386 内核 |
| `src/rv32/` | 放在 80386 旁边的 RV32 内核 |
| `rootfs/` | 根文件系统镜像 |
| `images/` | 生成的启动镜像（不纳入 git） |
| `script/bootstrap.sh` | 为当前步骤安装宿主工具 |
| `labs/i386/qemu/`、`labs/i386/bochs/`、`labs/i386/gem5/` | 80386 上的实验 |
| `labs/rv32/qemu/` | RV32 内核的 QEMU virt 启动脚本 |

编辑器的文件树和搜索都不显示 `reference/`。对照 `src/linux-0.11/` 时，按这个路径打开。

`src/linux-0.11/` 可以在当代计算机上编译。`labs/i386/qemu/launch.sh` 在 QEMU 里启动它，shell 可以运行 `ls`。按 F5 并选择 **Linux 0.11**，会停在第一条指令上，在编辑器里单步。`labs/i386/bochs/launch.sh` 在 Bochs 里打开同一套磁盘，那个终端里的调试器可以看 GDT、IDT、页表和 TSS。`labs/i386/gem5/launch.sh` 在 gem5 的乱序 CPU 上运行 `setup_paging`，并打印周期、L1 miss 和 store queue 停顿。`labs/i386/gem5/launch.sh timer` 继续进入内核，统计第一次 `_timer_interrupt`；没有根磁盘时，这次中断发生在 `panic("Unable to mount root")` 的自旋里。`labs/i386/gem5/launch.sh schedule` 统计第一次 `_schedule`，它从任务 0 切到任务 1。`labs/i386/gem5/launch.sh copy_page` 统计内联在 `_un_wp_page` 里的第一次 4KiB `rep movsl`。`labs/i386/gem5/launch.sh from START to END` 统计开跑前就定好的两个地址之间的一段，约束写在 [gem5](docs/zh/gem5.md)。`labs/rv32/qemu/launch.sh` 编译 RV32 内核，并在 QEMU 的 virt 板上把它启动到 shell。`help` 列出命令，shell 可以运行 `ls`。按 F5 并选择 **RV32**，会停在 `entry.S` 的 `_start`。该菜单里的 **Linux 0.11** 仍是 80386 的会话。两者都听 `127.0.0.1:1234`，一次启动一个。

`rootfs/hdc-0.11.img` 来自 [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) 的 `rootfs/hdc-0.11.img.xz`。SHA256：`c83a9837f3f804882a76756f2ff46349467bdf66ed44e301a6c5e124f414b6d1`。大小：127631360 字节。生成的 `Image` 不纳入 git。

## 许可证

`reference/linux-0.11/` 是 Linus Torvalds 的 Linux 0.11（1991 年版权），这些文件保持其原有条款。仓库里的其他文件使用 Apache License 2.0，见 [LICENSE](LICENSE)。
