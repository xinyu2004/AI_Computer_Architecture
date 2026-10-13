# 在 Bochs 里看机器状态

[English](../en/bochs.md)

这是学习的第 3 步。磁盘和 QEMU 相同：`src/linux-0.11/Image` 与 `rootfs/hdc-0.11.img`。Bochs 自带调试器，用来看寄存器、GDT、IDT、页表和 TSS。

这一步不改 `src/linux-0.11/`。断点在调试器里设置。内核里没有魔术断点指令。

按 F5 仍然调试 QEMU 里的机器，并停在 `bootsect.s` 的第一条指令。那个会话用来看源码和单步。这一步用来看描述符表和任务状态，F5 里看不到这些。

## 这台机器

`labs/bochs/bochsrc` 描述一台 16MB 的 PC。软盘是 `Image`。硬盘是 `rootfs/hdc-0.11.img`，410 柱面、16 磁头、38 扇区，与文件大小一致。没有网卡。

CPU 型号是 `pentium`。这是 Bochs 2.8 里最老的具名型号，它仍然执行这份内核使用的 386 保护模式指令。`ips=1000000` 让软盘的模拟延时在更少的指令之后结束，启动会在几秒内到达调试器。BIOS 是 `/usr/share/bochs/BIOS-bochs-legacy`。

## 怎样检查

在仓库根目录，运行过 `script/bootstrap.sh` 之后：

```bash
make -C src/linux-0.11
labs/bochs/launch.sh
```

脚本先打印这一次编译对应的断点，然后 Bochs 占用这个终端。调试器提示类似：

```text
Bochs internal debugger, type 'help' for help or 'c' to continue
```

本机有显示器时打开 SDL 窗口，窗口就是 VGA 控制台。命令打在这个终端里。没有显示器，或者显式指定时，控制台走 VNC：

```bash
labs/bochs/launch.sh vnc
```

Bochs 会打印它打开的端口，通常是 `5900`。VNC 客户端连上之后，调试器提示才出现。在你坐着的那台电脑上：

```bash
ssh -L 5900:127.0.0.1:5900 <user>@<这台机器>
```

然后打开 `localhost:5900`。

`q` 退出调试器。`help` 列出命令。如果 Bochs 报告磁盘镜像被锁住，说明另一个模拟器已经打开了 `rootfs/hdc-0.11.img`。关掉那个窗口，再运行这个脚本。

下面的地址是某一次链接的例子。`labs/bochs/launch.sh` 会打印你刚编译出来的地址。内核地址每次链接都会变。`lmsw` 的地址只在 `boot/setup.s` 改动时才变。setup 装在物理地址 `0x90200`，脚本再加上 `boot/setup.elf` 里 `lmsw` 的文件偏移。

## 保护模式

`boot/setup.s` 里，`lmsw` 置上 CR0.PE，下一条指令是 `ljmp $8,$0`。示例构建里，`lmsw` 的线性地址是 `0x90301`。用脚本打印出来的地址：

```text
pb 0x90301
c
```

CPU 停在 `lmsw` 上，PE 仍是 0。单步两次，下一条是那条远跳转：

```text
s
s
creg
sreg
info gdt 0 2
```

`creg` 里能看到 `PE`。`sreg` 里 CS 是选择子 `0x0008`：32 位代码段，基址 `0`，界限 `0x7fffff`。`info gdt 0 2` 显示 `lgdt` 装入的表。第 0 项是空描述符。选择子 8 是上面的代码段。选择子 `0x10` 是数据段，基址同样是 0，界限同样是 `0x7fffff`。GDTR 的基址是 setup 里的那张 GDT（`lmsw` 在 `0x90301` 时，基址是 `0x90314`）。界限 `0x800` 是 `setup.s` 里的临时表。不加范围的 `info gdt` 会把界限内的每一项都打出来。

## 分页

`boot/head.s` 的 `setup_paging` 在物理地址 0 写下页目录，然后置上 CR0.PG。脚本会打印它的地址（示例链接是 `0x5450`）：

```text
pb 0x5450
c
creg
```

第一条指令是 `movl $1024*5,%ecx`。这时 PG 仍是 0。单步到 `movl %eax,%cr0` 执行完，再运行 `creg`。PG 已置位，CR3 为 0，也就是页目录。`page 0` 翻译线性地址 0。这张目录把前 16MB 做恒等映射，四张页表，每张 4MB。

## 时钟中断

`setup.s` 把第一片 8259 编程成 IRQ0 使用向量 `0x20`。处理函数是 `kernel/system_call.s` 里的 `_timer_interrupt`。脚本会打印它的地址（示例链接是 `0x6ee4`）：

```text
pb 0x6ee4
c
info idt 0x20
sreg
```

`info idt 0x20` 显示一个 32 位中断门，DPL 为 0，目标就是这个处理函数。栈上是中断返回后要回到的地址。

## 任务切换

`_schedule` 决定接下来运行哪个任务。真正的切换是 `include/linux/sched.h` 里 `switch_to` 宏中的 `ljmp`。脚本会打印 `_schedule` 的地址（示例链接是 `0x6814`）：

```text
pb 0x6814
c
info tss
sreg
```

`info tss` 打印任务状态段。`switch_to` 里的远跳转执行之后，`sreg` 里的 TR 指向当前任务的 TSS。
