# 宿主工具

[English](../en/tools.md)

编译或启动之前，在仓库根目录运行一次：

```bash
script/bootstrap.sh
```

这个脚本安装当前已经开始的步骤要用的工具，并说明每个包属于哪一步。在 Debian 和 Ubuntu 上，它执行一次 `apt-get install`。在其他系统上，它打印同一份清单后停止，方便你用自己的包安装这些命令。

## 第 2 步，编译内核

`build-essential` 提供 `make`、`as` 和 `ld`。`make -C src/linux-0.11` 用它们把内核做成 `src/linux-0.11/Image`。

`gcc-multilib` 提供 `gcc -m32`。宿主 gcc 默认生成 64 位目标文件。这份内核是 32 位 80386 代码，编译器必须能发出这种代码。

## 第 2 步，运行

`qemu-system-x86` 提供 `qemu-system-i386`。`labs/qemu/launch.sh` 启动这个模拟器，把 `Image` 当作软盘，把 `rootfs/hdc-0.11.img` 当作第一块硬盘。控制台是 QEMU 的窗口。

## 后面的步骤

Bochs 是第 3 步的工具，用来看寄存器、GDT、IDT、页表和 TSS。gem5 是第 4 步的工具，用来看周期和 cache miss。`script/bootstrap.sh` 现在还不安装它们。
