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

`qemu-system-x86` 提供 `qemu-system-i386`。`labs/i386/qemu/launch.sh` 启动这个模拟器，把 `Image` 当作软盘，把 `rootfs/hdc-0.11.img` 当作第一块硬盘。本机桌面用 GTK 窗口，SSH 转发的 X11 用 SDL 窗口。

`gdb` 是编辑器使用的调试器。按 F5，选择 **Linux 0.11**。它启动的机器和 `labs/i386/qemu/launch.sh` 相同，CPU 暂停，并停在 `bootsect.s` 的第一条指令。源码、单步和寄存器都在编辑器里。单独运行 `labs/i386/qemu/launch.sh` 仍然直接进入 shell。

窗口没有出来时，改用 VNC。客户端装在你坐着的那台电脑上：

```bash
labs/i386/qemu/launch.sh vnc
```

QEMU 监听 `127.0.0.1:5901`。用 `ssh -L 5901:127.0.0.1:5901 <user>@<这台机器>` 转发该端口，再打开 `localhost:5901`。

## 第 3 步，机器状态

`bochs` 提供模拟器和它自带的调试器。`labs/i386/bochs/launch.sh` 启动的软盘和硬盘与 `labs/i386/qemu/launch.sh` 相同。调试器提示留在那个终端里，用来看寄存器、GDT、IDT、页表和 TSS。[看什么](bochs.md)。

`bochs-sdl` 提供 SDL 窗口，有显示器时它就是 VGA 控制台。`bochsbios` 提供 `BIOS-bochs-legacy`。`vgabios` 提供 `vgabios.bin`。没有显示器时，`bochs` 主包里已经带了 VNC 控制台，端口是 5900。

## 第 4 步，周期和 miss

`python3-dev` 提供 `Python.h`。gem5 会把这个 Python 嵌进去。`python3-venv` 提供 `python3 -m venv`。`labs/i386/gem5/build.sh` 建立 `.gem5/.venv`，并把 scons 装在那里。`m4` 是 gem5 编译时要跑的宏处理器。

gem5 本身不是 apt 包。`labs/i386/gem5/build.sh` 把固定的 v25.1.0.1 克隆到 `.gem5/`，打上 `labs/i386/gem5/linux-0.11-x86.patch`，并编译出 `.gem5/build/X86/gem5.opt`。这个目录不纳入 git。

`labs/i386/gem5/launch.sh` 加载 `src/linux-0.11/tools/system.elf` 并运行 `setup_paging`。它模拟的机器写在 `labs/i386/gem5/linux-0.11-x86.py` 里。`labs/i386/gem5/launch.sh timer`、`schedule` 和 `copy_page` 从 `startup_32` 开始，分别统计第一次时钟中断、第一次 `_schedule`，以及第一次内联的 `copy_page`。每次都打印周期、L1 miss 和流水线停顿。[量什么](gem5.md)。编译时可能会警告 GCC 比 gem5 源码里写的范围更新。它编出来的二进制就是 `launch.sh` 运行的那个。

## 第 5 步，RV32

`gcc-riscv64-unknown-elf` 提供 `riscv64-unknown-elf-gcc`。同一个编译器带有 32 位 multilib。`make -C src/rv32` 使用 `-march=rv32gc -mabi=ilp32`，所以目标文件是 RV32，尽管命令名字里写着 riscv64。

`qemu-system-riscv` 提供 `qemu-system-riscv32`。`labs/rv32/qemu/launch.sh` 用 `-machine virt -cpu rv32` 启动这个模拟器，UART 停在 shell。在那个提示符下输入 `help` 会列出盘上的程序。如果某个 Debian 或 Ubuntu 源里这个二进制还装在 `qemu-system-misc` 中，`script/bootstrap.sh` 会改装那个包。[RV32 内核做什么](rv32.md)。

`gdb-multiarch` 是这份内核的调试器。按 F5，选择 **RV32**。它停在 `src/rv32/entry.S` 的 `_start`，也就是内核的第一条指令。UART 输出在那个任务的终端里。单独运行 `labs/rv32/qemu/launch.sh` 仍然进入 shell。

同一个包也提供 `qemu-system-riscv64`。`make -C src/rv64` 使用 `-march=rv64gc -mabi=lp64`。`labs/rv64/qemu/launch.sh` 启动这份内核。按 F5 并选择 **RV64**，停在 `src/rv64/entry.S` 的 `_start`。Linux 0.11、RV32 和 RV64 都听 `127.0.0.1:1234`，一次启动一个。
