# 用 Linux 0.11 学习计算机体系结构

[English](README.md)

这个仓库面向想用 Linux 0.11 学习计算机体系结构的人。这份内核小到可以整份读完，写于 1991 年，面向 80386 和 PC 设备。学习时先把它接到当代计算机上，再顺着下面的顺序看下去。

## 步骤

1. **从原版读起。** `reference/linux-0.11/` 保存未经修改的 Linux 0.11。后面的改动都回到这里对照。
2. **在当代计算机上编译，并在 QEMU 里运行。** 源码改到能用现在的工具链编译。改一行、重新编译，就可以观察操作系统在做什么：能否启动、系统调用是否返回、进程是否出现。
3. **在 Bochs 里看机器状态。** 同一个系统进入保护模式、响应中断、切换任务时，观察寄存器、GDT、IDT、页表和 TSS 的变化。
4. **在 gem5 里看执行代价。** 沿同一段内核路径，观察周期、cache miss 和流水线停顿。
5. **再看别的架构。** x86 走通之后，保持内核短小，先加入 RISC-V，再加入 ARM，对照哪些属于某一种架构，哪些是操作系统本身。

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
| `src/linux-0.11/` | 要编译、要修改的内核 |
| `rootfs/` | 根文件系统镜像 |
| `images/` | 生成的启动镜像（不纳入 git） |
| `labs/qemu/`、`labs/bochs/`、`labs/gem5/` | 各环境上的实验 |

这一版只包含冻结的原始内核。可编译的内核树、根文件系统和实验目录随后再补。

## 许可证

`reference/linux-0.11/` 是 Linus Torvalds 的 Linux 0.11（1991 年版权），这些文件保持其原有条款。仓库里的其他文件使用 Apache License 2.0，见 [LICENSE](LICENSE)。
