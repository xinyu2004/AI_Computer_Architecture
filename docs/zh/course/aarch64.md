# AArch64 上的停法

[English](../../en/course/aarch64.md)

[问题表](../course.md)

启动是 `labs/aarch64/qemu/launch.sh`。UART 在这个终端里。F5 选 **AArch64**，停在 `entry.S` 的 `_start`，再下断点。断在函数上。地址以这次编译为准。

图上这一列的铭牌是 [EL1](../aarch64_instructions.md#pstate) 和 [TTBR0_EL1](../aarch64_instructions.md#ttbr0-el1)。AArch64 是 ARMv8 的 64 位状态。见 [机制图](../mech.md)。

这是另一份内核，不是 ARMv7 加一层条件编译。异常级、页表和系统寄存器都不共用。

## UART 上先看这段

```text
aarch64
currentel 0x00000004
uart 0x09000000
timebase 62500000
ttbr0 on
disk 0x0a000000
$ ls
ls
sh
echo
sleep
$ echo hello
hello
$ echo hello
hello
$ nosuch
not found
$ sleep 10
$ exit
```

`sleep 10` 停 10ms。`uart`、`timebase` 和 `disk` 来自这一次的设备树，QEMU 10.2.1 打出过上面的数。`timebase` 是 `CNTFRQ_EL0`。`currentel` 的值 `0x4` 表示 EL1。`exit` 让 QEMU 退出。

盘上的程序仍是 `src/user/` 里的那四份 C。系统调用包装在 `src/aarch64/user.h`：`svc #0`，调用号在 `x8`，参数在 `x0` 到 `x2`。指针是 64 位，栈按 16 字节对齐。

## 停在哪

### 上电 {#boot}

F5 停在 `_start`。镜像链在 `0x40080000`，QEMU 的引导桩跳到这里，`x0` 是设备树。继续运行后，UART 打出 `currentel 0x00000004`，也就是 EL1。

### 留在 EL1 {#mode}

这一次启动已经在 EL1h。断在 `main.c` 里读 `CurrentEL` 的那条。读到的是 `0x4`。

### TTBR0_EL1 {#pages}

断在 `vm.c` 的 `vm_on`，看写 `TTBR0_EL1` 的那条。`T0SZ` 是 25，虚拟地址 39 位，4KiB 粒度，从第 1 级走起。单步过去，UART 接着打出 `ttbr0 on`。

### EL0 {#privilege}

`$` 出现以后，断在 `exec.c` 的 `syscall`。敲回车。进来时 `SPSR_EL1` 是 0，EL0t。调用号在 `x8`。

### 时钟 {#timer}

`$` 已经出现，还没敲字。断在 `traps.c` 的 `irq_handler`。`gic_iar` 返回 30。用户态里的这一次走 `VBAR_EL1` 偏移 `0x480` 的槽。内核自己在等的时候走偏移 `0x280`。处理函数改写 `CNTP_CVAL_EL0`。

设备树里的中断控制器是 GICv2，`compatible` 为 `arm,cortex-a15-gic`。认领中断的仍是 `GICC_IAR`。

### svc {#ecall}

断在 `trap.S` 的 `svc_entry`。敲 `echo hello`。这是 `VBAR_EL1` 偏移 `0x400` 的槽，来自 EL0 的 AArch64 同步异常。`ESR_EL1` 的 EC 是 `0x15`。shell 的 `exec` 进来时 `x8` 是 3。`ELR_EL1` 已经指向 `svc` 的下一条。UART 打出 `hello`。

### swtch {#switch}

断在 `swtch.S` 的 `swtch`。再敲 `echo hello`。保存的是 `x19` 到 `x30`，以及 `sp`。`echo` 退出时再进一次，回到 shell 下一次读行。

### copy_page {#copy}

断在 `mem.c` 的 `copy_page`。再敲一次 `ls`。函数走一遍，512 个 64 位字，仍是 4096 字节。

敲 `nosuch`。UART 打出 `not found`。`swtch` 和 `copy_page` 都不进。

### 字节 {#byte}

`$` 出现以后，断在 `uart.c` 的 `uart_getc`。敲一个字母。这仍是 PL011：标志寄存器偏移 `0x18` 的第 4 位表示接收队列空，数据在偏移 0。定时器把等待叫醒。

### 一行 {#line}

断在 `uart.c` 的 `read_line`。敲 `echo hello`。回车之后整行交给 shell。

### 名字 {#name}

断在 `fs.c` 的 `lookup`。再敲 `echo hello`。参数是 `echo`。返回后 `name_block` 是这个文件的扇区号。

### 扇区 {#block}

断在 `fs.c` 的 `bread`。第一次 `echo hello` 会进 `virtio_read`。再敲一次同样的命令，`bread` 交还缓存里的那一块。

### 磁盘完成 {#disk}

断在 `irq_handler`，再敲一条还没装入过的命令。`gic_iar` 返回 48，和 ARMv7 这次启动读到的是同一个 GIC 号。`virtio_ack` 写应答，`gic_eoi` 结束中断。再敲一次 `echo hello` 不会再进来。
