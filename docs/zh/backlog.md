# Backlog

这里的事项不属于 README 里的学习步骤。第 3 到第 5 步仍是 Bochs、gem5，然后是别的架构。

## 生成根磁盘

`rootfs/hdc-0.11.img` 是 QEMU 使用的教学盘。它不是内核编译的产物，也不在 1991 年的内核包里。仓库里的这份文件来自 [hello-os/linux-0.11](https://github.com/hello-os/linux-0.11) 的 `rootfs/hdc-0.11.img.xz`。

现在先不生成这张盘。以后生成时，用脚本建立分区表和 Minix 文件系统，再放入 `/dev`、`/etc` 和用户程序。这些程序（`/bin/sh`、`/bin/ls` 以及其余命令）是另一套源码。这个内核执行的是带 1024 字节头的 a.out ZMAGIC，所以不能把宿主 GCC 的输出拷进磁盘。现有的移植都把这套用户态留成已经编好的程序。

在那个脚本出现之前，QEMU 继续使用上面的教学盘。

[English](../en/backlog.md)
