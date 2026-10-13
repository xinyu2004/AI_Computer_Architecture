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

`qemu-system-x86` 提供 `qemu-system-i386`。`labs/qemu/launch.sh` 启动这个模拟器，把 `Image` 当作软盘，把 `rootfs/hdc-0.11.img` 当作第一块硬盘。本机桌面用 GTK 窗口，SSH 转发的 X11 用 SDL 窗口。

`gdb` 是编辑器使用的调试器。按 F5，选择 **Linux 0.11**。它启动的机器和 `labs/qemu/launch.sh` 相同，CPU 暂停，并停在 `bootsect.s` 的第一条指令。源码、单步和寄存器都在编辑器里。单独运行 `labs/qemu/launch.sh` 仍然直接进入 shell。

窗口没有出来时，改用 VNC。客户端装在你坐着的那台电脑上：

```bash
labs/qemu/launch.sh vnc
```

QEMU 监听 `127.0.0.1:5901`。用 `ssh -L 5901:127.0.0.1:5901 <user>@<这台机器>` 转发该端口，再打开 `localhost:5901`。

## 第 3 步，机器状态

`bochs` 提供模拟器和它自带的调试器。`labs/bochs/launch.sh` 启动的软盘和硬盘与 `labs/qemu/launch.sh` 相同。调试器提示留在那个终端里，用来看寄存器、GDT、IDT、页表和 TSS。[看什么](bochs.md)。

`bochs-sdl` 提供 SDL 窗口，有显示器时它就是 VGA 控制台。`bochsbios` 提供 `BIOS-bochs-legacy`。`vgabios` 提供 `vgabios.bin`。没有显示器时，`bochs` 主包里已经带了 VNC 控制台，端口是 5900。

## 后面的步骤

gem5 是第 4 步的工具，用来看周期和 cache miss。`script/bootstrap.sh` 现在还不安装它。
