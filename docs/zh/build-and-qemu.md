# 在当代计算机上编译，并在 QEMU 里运行

[English](../en/build-and-qemu.md)

这是学习的第 2 步。做完之后，内核可以用当代工具链编译，并在 QEMU 里启动。改一行、重新编译，运行中的系统会显示出它有没有启动、系统调用有没有返回、进程有没有出现。

`reference/linux-0.11/` 保持 1991 年的原样。要编译的内核在 `src/linux-0.11/`。这两棵树之间的 diff 就是这一步的全部改动。

做法跟着已经能在当代机器上启动 Linux 0.11 的移植，主要是 [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11)。那些树说明有一条走得通的路。本仓库不用它们替换 `src/linux-0.11/`。调度、内存管理和系统调用保持原样。

## 为什么 1991 年的树编不过

`reference/linux-0.11/Makefile` 仍写着 1991 年的工具和目标文件格式。

宿主是 64 位计算机。这份内核是 32 位 80386 代码。宿主上直接运行 `gcc` 会得到 64 位目标文件，而旧选项 `-fcombine-regs` 和 `-mstring-insns` 在现在的 GCC 里已经没有了。

`boot/bootsect.s` 和 `boot/setup.s` 是 as86 汇编，当时用 `as86` 和 `ld86` 构建。现在的 binutils 提供的是 GNU `as`。`boot/head.s` 和其余内核汇编本来就是 GNU as，所以对不上学生机器上汇编器的是这两个引导文件。

`tools/build.c` 把引导扇区、setup 和内核打成软盘镜像 `Image`。它按 a.out 头来读（`MINIX_HEADER` 32 字节，或旧的 `GCC_HEADER` 1024 字节）。现在的 GCC 写出的是 ELF，这些头长度不再对得上文件。

内核源码里也没有 shell。只启动 `Image` 到不了登录。还要挂上一份根文件系统镜像，原 Makefile 里的 `ROOT_DEV` 已经这么假设了（`/dev/hd6`）。

## 这一步改了什么

### 工具链

Makefile 使用 `gcc -m32`、`as --32` 和 `ld -m elf_i386`，并去掉当前 GCC 会拒绝的选项。`-std=gnu89 -fno-stack-protector -fno-builtin -nostdinc -fno-pic` 保留 1991 年源码所用的方言和独立编译方式。栈保护和位置无关代码会插入原内核没有的调用和寻址。

### 引导汇编

`bootsect.s` 和 `setup.s` 从 as86 改写成 GNU as。加载地址没有改。

as86 写的是 `mov ax,#BOOTSEG` 和 `jmpi go,INITSEG`。GNU as 不接受这套语法，而且在 64 位宿主上，文件如果不以 `.code16` 开头，就会按 32 位指令汇编。BIOS 进入软盘引导扇区时仍是 16 位实模式，所以新文本使用 `.code16`、`movw $BOOTSEG, %ax` 和 `ljmp $INITSEG, $go`。`ljmp` 的操作数顺序和 as86 的 `jmpi` 相反。跳转仍是 `0x9000:go`。

地址保持不变，是因为后面的引导仍按这些位置找东西。BIOS 把引导扇区放在 `0x7C00`。引导扇区再把自己复制到 `0x90000`，把 setup 放到 `0x90200`，把内核放到 `0x10000`。`setup.s` 把 BIOS 参数写在 `0x90000`，`head.s` 从那里读取。`head.s` 本来就是 GNU as，保护模式的路径不变。as86 原文留在 `reference/` 里对照。

### 软盘镜像

`objcopy -O binary` 去掉 ELF 头。`tools/build.c` 再写成和原来一样的 `Image` 布局：512 字节的引导扇区、setup 扇区，以及内核。QEMU 启动的是这个镜像格式。ELF 头是当代链接器的产物，不属于 1991 年的引导协议。

### 当前 GCC 拒绝编译的 C

改动限于当前 GCC 编不过的写法，例如旧的 `extern inline` 和隐式函数声明。这些修改是为了让文件能编译。进程调度、分页和系统调用路径不在这一步里重写。

### 根文件系统和 QEMU

根镜像是 [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) 里的教学盘 `rootfs/hdc-0.11.img.xz`，解压为 `rootfs/hdc-0.11.img`。

SHA256：`c83a9837f3f804882a76756f2ff46349467bdf66ed44e301a6c5e124f414b6d1`

大小：127631360 字节。

QEMU 使用 16MB 内存，把 `Image` 当作启动软盘，把该文件当作第一块硬盘。16MB 与这份内核和这张根镜像惯用的内存大小一致。`include/linux/config.h` 里的键盘是 `KBD_US`，和那个移植一样，因为 QEMU 送来的是美式键盘。

### 当前 GCC 会编错的内联汇编

1991 年的一些 `asm` 把寄存器写成输入，指令却会改掉它（`rep movs`、`lodsl`、`bts`）。GCC 15 会把这个寄存器继续交给周围的 C 使用。hello-os 的树不用 `-O` 来避开这一点。本仓库仍用 `-O2`，并把被改掉的寄存器标成输出，作用相同。决定 shell 能否出现的两处是 `fs/buffer.c` 里的 `COPYBLK`（缺页读入 `/bin/sh` 时，目标地址多走了 2048 字节，写进了下一个任务），以及 `fs/bitmap.c` 里的 `find_first_zero`（`lodsl` 让 `esi` 指向下一个位图字，于是 `new_block` 以 “bit already set” 停机）。

## 如何检查

在仓库根目录先运行一次 `script/bootstrap.sh`，装好宿主工具。[它安装什么，以及为什么](tools.md)。然后编译软盘镜像。

```bash
make -C src/linux-0.11
```

`tools/build` 会打印 `Root device is (3, 1)`。结果是 `src/linux-0.11/Image`。

把该镜像当作软盘，把 `rootfs/hdc-0.11.img` 当作第一块硬盘启动：

```bash
labs/qemu/launch.sh
```

内核把控制台写在 VGA 内存里。`-nographic` 会是空白。本机桌面会打开 GTK 窗口。经 SSH 转发 X11 时打开 SDL 窗口：转发过来的显示编号从 10 开始（`localhost:10.0`），脚本以此来和本机的 `:0` 区分。这条路径同时关闭 OpenGL，从而避开 XLaunch 这类 X 服务器打出的 GTK 显示器警告。

窗口没有出来时，用 VNC 启动同一台机器：

```bash
labs/qemu/launch.sh vnc
```

这时 QEMU 监听 `127.0.0.1:5901`。在你坐着的那台电脑上转发该端口，再用 VNC 客户端打开 `localhost:5901`：

```bash
ssh -L 5901:127.0.0.1:5901 <user>@<这台机器>
```

想改掉自动选择时，同一个脚本也接受 `gtk` 或 `sdl`。没有 `DISPLAY`、也没有 Wayland 显示时，`labs/qemu/launch.sh` 自己就会选 VNC。

硬盘和软盘参数始终是：

```bash
qemu-system-i386 -m 16M -boot a -nic none \
  -drive file=src/linux-0.11/Image,if=floppy,format=raw \
  -drive file=rootfs/hdc-0.11.img,if=ide,format=raw,index=0
```

控制台会出现 `[/usr/root]#`。输入 `ls /`，列出 `bin`、`dev`、`etc`、`shoelace`、`usr` 和 `var`。

## 调试

单独运行 `labs/qemu/launch.sh` 和调试是两件事。脚本负责把系统启动到 shell。单步和寄存器在编辑器里，暂停着的 QEMU 窗口本身没有这些信息。

按 F5，选择 **Linux 0.11**。`.vscode/tasks.json` 在后台启动同一个 `launch.sh`，并暂停 CPU。`.vscode/launch.json` 把 GDB 连到 `127.0.0.1:1234`，载入符号，停在这个系统的第一条指令：

```
bootsect.s: movw $BOOTSEG, %ax
```

这条指令在 `0x7C00`。断点是硬件断点，BIOS 把引导扇区载入内存之后它还在。会话停住时，工具栏是继续而不是暂停，这时才能单步。监视里的程序计数器写 `$pc`，不要写 `pc`。QEMU 窗口和单独运行 `launch.sh` 用同一套选择：本机桌面是 GTK，SSH 转发的 X11 是 SDL，没有显示器时是 VNC。源码、单步和寄存器在编辑器里。

`boot/bootsect.elf` 对应跑在 `0x7C00` 的指令。从第一条 `movw` 单步，可以走过那段把自己复制走的循环。接下来 `ljmp` 到 `0x9000`，段地址变了。GDB 之后只报告段内偏移，`bootsect.s` 后面的行就对不上了。

`boot/setup.elf` 放在 `0x90200`，对应 `setup.s`。`tools/system.elf` 对应物理地址 0 的 `head.s` 和 C 源码，保护模式从这里开始。C 的名字保留前导下划线，所以 C 里的内核入口是 `_main`。这份符号先不载入，它的地址范围盖住 `0x7C00`，会把引导扇区挡住。

`-g` 留在这些 ELF 里。软盘 `Image` 仍是纯二进制，编译仍是 `-O2`，所以后面的 C 源码行有时会被跳过。BIOS 没有符号。

如果 F5 提示磁盘被锁住，说明已经有一个 QEMU 窗口打开着 `rootfs/hdc-0.11.img`。关掉它，再开始这次调试。单独运行 `labs/qemu/launch.sh` 不会暂停，也不会打开 GDB 端口。

`diff -rq reference/linux-0.11 src/linux-0.11` 看到的是上面的工具链、引导语法、镜像打包和编译器修正。调度、分页和系统调用本身仍是 1991 年的代码。Bochs 和 gem5 是后面的步骤。
