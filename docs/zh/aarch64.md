# virt 板上的 AArch64

[English](../en/aarch64.md)

这是第 5 步里 ARM 的第二份内核。`src/aarch64/` 是 Cortex-A57 上的内核，不和 `src/armv7/` 共用实现。它打开 4KiB 页表，打印机器信息，然后留下同一个 shell。停法在 [AArch64 这一遍](course/aarch64.md)。ARMv7 见 [ARMv7](armv7.md)。

```bash
labs/aarch64/qemu/launch.sh
```

脚本用 `aarch64-linux-gnu-gcc -mcpu=cortex-a57` 编译，写好 `images/aarch64.img`，并用 `qemu-system-aarch64 -machine virt -cpu cortex-a57` 启动原始镜像 `src/aarch64/kernel.bin`。按 F5 并选择 **AArch64**，停在 `_start`。五个调试会话都听 `127.0.0.1:1234`，一次启动一个。

## 这次运行打印什么

```text
aarch64
currentel 0x00000004
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$
```

`currentel` 的 `0x4` 是 EL1。`timebase` 是 `CNTFRQ_EL0`。数字来自 QEMU 10.2.1 的这一次启动。用户程序是 ELF64，链在 `0x10000`，指针 64 位。

## 这块板子

引导桩把镜像放在 `0x40080000`，CPU 已经在 EL1h，`x0` 是设备树。内核链在这个地址。UART、GICv2 和 virtio 的地址仍从设备树读。这棵树上的中断控制器 `compatible` 是 `arm,cortex-a15-gic`。关机仍是 PSCI `0x84000008`，走 `hvc`。

## 五件事

| 事情 | 位置 | 和 ARMv7 内核相比 |
| --- | --- | --- |
| 启动 | `entry.S`、`main.c` | `_start` 在 EL1。异常向量在 `VBAR_EL1`。 |
| 页表 | `vm.c` | `TTBR0_EL1`，4KiB 粒度，`T0SZ` 为 25。 |
| 时钟 | `traps.c` | `CNTP_CVAL_EL0`。GIC 号这次仍是 30。 |
| 系统调用 | `trap.S` | `svc` 来自 EL0。`ESR_EL1` 的 EC 是 `0x15`，调用号在 `x8`。 |
| 复制一页 | `mem.c` | `copy_page` 拷 512 个 64 位字，仍是 4KiB。 |

名字本身看不出作用的指令在[一页](aarch64_instructions.md)里。ARMv7 的名字在 [ARMv7 那一页](armv7_instructions.md)。80386 的名字在 [80386 那一页](i386_instructions.md)。
