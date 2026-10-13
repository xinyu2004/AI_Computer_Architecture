# 保护模式

[English](../../en/cases/protected-mode.md)

[目录](../cases.md)

## 这一章要看懂什么

CPU 从实模式进入保护模式，靠的是 CR0 的 PE 位和一张 GDT。`lmsw` 置上 PE。下一条远跳转装入选择子 8，代码段和数据段的基址都是 0。

## 读哪里

`src/linux-0.11/boot/setup.s` 第 129 行的 `lmsw %ax`，以及它后面的 `ljmp $8,$0`。同一文件里，`lgdt` 装入的是一张临时表。

## 怎么启动

在仓库根目录：

```bash
make -C src/linux-0.11
labs/bochs/launch.sh
```

命令打在这个终端里。VGA 窗口是控制台。脚本会打印 `lmsw` 的线性地址。这一次链接的例子是 `0x90301`。用脚本打出来的那个地址：

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

机器、VNC 和磁盘锁见 [Bochs](../bochs.md)。

## 该看到什么

`creg` 里 PE 已置位。`sreg` 里 CS 是选择子 `0x0008`：32 位代码段，基址 `0`，界限 `0x7fffff`。`info gdt 0 2` 里，第 0 项是空描述符，选择子 8 是代码段，选择子 `0x10` 是数据段，基址同样是 0，界限同样是 `0x7fffff`。这张表是 `setup.s` 里的临时 GDT。内核自己的 GDT 要到 `boot/head.s` 才装入。
