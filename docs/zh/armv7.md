# virt 板上的 ARMv7

[English](../en/armv7.md)

这是第 5 步里 ARM 的第一份内核。`src/linux-0.11/` 仍然是 80386 内核。`src/armv7/` 是 Cortex-A15 上的另一份内核。它打开短描述符页表，打印机器信息，然后留下一个 shell。盘上的程序是 `ls`、`echo` 和 `sleep`。同一批问题在[课程](course.md)里，这一台机器的停法在 [ARMv7 这一遍](course/armv7.md)。AArch64 是旁边的另一份内核，见 [AArch64](aarch64.md)。

Bochs 和 gem5 的 x86 运行都不变。这份内核单独启动：

```bash
labs/armv7/qemu/launch.sh
```

脚本用 `arm-none-eabi-gcc -mcpu=cortex-a15` 编译，写好 `images/armv7.img`，并用 `qemu-system-arm -machine virt -cpu cortex-a15` 启动原始镜像 `src/armv7/kernel.bin`。控制台是 PL011。`exit` 关机。按 F5 并选择 **ARMv7**，会停在 `entry.S` 的 `_start`。Linux 0.11、RV32、RV64、ARMv7 和 AArch64 都听 `127.0.0.1:1234`，一次启动一个。

## 这次运行打印什么

```text
armv7
cpsr 0x600001d3
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$
```

完整的 `ls`、`echo`、`nosuch`、`sleep` 和 `exit` 在[停法](course/armv7.md)里。`uart`、`timebase` 和 `disk` 是这次启动从设备树读到的值。上面的数字来自 QEMU 10.2.1。`timebase` 是 `CNTFRQ`。

`images/armv7.img` 里是链接在 `0x10000` 的 ELF32 程序。内核把 `sh` 载入用户态。`fork` 拷走一页栈，`exec` 换进敲下的程序，`exit` 再换回来。

## 这块板子

`virt` 是 QEMU 的 ARM 机器。CPU 是 `-cpu cortex-a15`。引导桩把原始镜像放在 `0x40010000`，CPU 已经在 SVC，`r2` 是设备树。内核就链在这个地址。`fdt.c` 顺着这棵树走。UART 是 `arm,pl011`。中断控制器是 `arm,cortex-a15-gic`，GICv2。第一台 virtio-mmio 是磁盘，启动行把它挂在 `virtio-mmio-bus.0` 上。关机是 PSCI `SYSTEM_OFF`，功能号 `0x84000008`，走设备树里的 `hvc`。

## 五件事

| 事情 | 位置 | 和 80386 内核相比 |
| --- | --- | --- |
| 启动 | `entry.S`、`main.c` | `_start` 在 SVC。没有引导扇区。 |
| 页表 | `vm.c` | 短描述符，`TTBR0`，`TTBCR.EAE` 为 0。没有 GDT。 |
| 时钟 | `traps.c` | `CNTP_CVAL` 在进提示符之前装好。GIC 号来自设备树，这次是 30。 |
| 任务切换 | `swtch.S` | `exec` 保存 shell 并进入子进程。两个任务都留在 SVC。 |
| 复制一页 | `mem.c` | `copy_page` 拷 1024 个 32 位字，共 4KiB。 |
