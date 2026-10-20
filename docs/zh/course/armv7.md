# ARMv7 上的停法

[English](../../en/course/armv7.md)

[问题表](../course.md)

启动是 `labs/armv7/qemu/launch.sh`。UART 在这个终端里。F5 选 **ARMv7**，停在 `entry.S` 的 `_start`，再下断点。断在函数上。地址以这次编译为准。

图上这一列的铭牌是 [SVC](../armv7_instructions.md#cpsr) 和 [TTBR0](../armv7_instructions.md#ttbr0)，见 [机制图](../mech.md)。

## UART 上先看这段

```text
armv7
cpsr 0x600001d3
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

`sleep 10` 停 10ms，终端里像没停。要看见停顿，用 `sleep 1000`。`uart`、`timebase` 和 `disk` 来自这一次的设备树，QEMU 10.2.1 打出过上面的数。`timebase` 是 `CNTFRQ` 读回来的值。`exit` 让 QEMU 退出。

`fork` 在 `exec.c` 里调用 `copy_page`。`exec` 装入程序后调用 `swtch`，子进程从自己的 `_start` 跑。`exit` 再 `swtch` 回 shell。回来用的是 `fork` 时留下的页面，内核不会把 `sh` 从磁盘再读一遍。

## 停在哪

### 上电 {#boot}

F5 停在 `_start`。QEMU 的引导桩已经把 CPU 放在 SVC，再跳到这里。`r2` 是设备树。继续运行后，UART 打出 `cpsr 0x600001d3`。低 5 位是 `0x13`，这就是 SVC。

### 留在 SVC {#mode}

这一次启动不再改模式。断在 `main.c` 里读 `cpsr` 的那条。打出来的值低 5 位仍是 `0x13`。

### TTBR0 {#pages}

断在 `vm.c` 的 `vm_on`，看写 `TTBR0` 的那条。`TTBCR.EAE` 保持 0，页表是短描述符，4KiB 小页。单步过去，UART 接着打出 `ttbr0 on`。

### 用户态 {#privilege}

`$` 出现以后，断在 `exec.c` 的 `syscall`。敲回车。进来时 `spsr` 的低 5 位是 `0x10`，用户态。调用号在 `r7`。

### 时钟 {#timer}

`$` 已经出现，还没敲字。断在 `traps.c` 的 `irq_handler`。`gic_iar` 的返回值是 30，这是设备树里非安全物理定时器的 GIC 号。`current_name` 是 `sh`。处理函数改写 `CNTP_CVAL`，这一拍是 `CNTFRQ / 100`。

这块板从安全态的 SVC 进来。`gic_enable` 把中断放进 Group 1。CPU 接口要同时打开 Group 1，并且置上 AckCtl，安全态读 `GICC_IAR` 才会返回这个号。AckCtl 没置上时，读到的是 1022，中断一直挂着。

### svc {#ecall}

断在 `trap.S` 的 `svc_entry`。敲 `echo hello`。shell 的 `exec` 进来时 `r7` 是 3。`echo` 写 UART 时 `r7` 是 1。硬件把返回地址留在 `lr`，已经指向 `svc` 的下一条。UART 打出 `hello`。

### swtch {#switch}

断在 `swtch.S` 的 `swtch`。再敲 `echo hello`。第一次留下的是 shell，装入的是 `echo` 的 `_start`。保存的是 `r4` 到 `r11`，以及 `lr` 和 `sp`。`echo` 退出时再进一次，回到 shell 下一次读行。

### copy_page {#copy}

断在 `mem.c` 的 `copy_page`。再敲一次 `ls`。函数走一遍，源是 shell 的栈页，目标是子进程的栈页，1024 个字，4096 字节。

敲 `nosuch`。UART 打出 `not found`。`swtch` 和 `copy_page` 都不进。

### 字节 {#byte}

`$` 出现以后，断在 `uart.c` 的 `uart_getc`。敲一个字母。标志寄存器在基址加 `0x18`，第 4 位为 1 表示接收队列空。数据寄存器在偏移 0。接收不是中断：定时器把等待叫醒，再读这个寄存器。

### 一行 {#line}

断在 `uart.c` 的 `read_line`。敲 `echo hello`。字母在这里收成一行，回车之后整行交给 shell。

### 名字 {#name}

断在 `fs.c` 的 `lookup`。再敲 `echo hello`。参数是 `echo`。返回后 `name_block` 是这个文件的扇区号。

### 扇区 {#block}

断在 `fs.c` 的 `bread`。第一次 `echo hello` 会进 `virtio_read`。再敲一次同样的命令，`bread` 交还缓存里的那一块，不再进 `virtio_read`。

### 磁盘完成 {#disk}

断在 `irq_handler`，再敲一条还没装入过的命令，例如第一次 `echo hello`。`gic_iar` 返回 48，这是设备树里第一台 virtio 的 GIC 号。`virtio_ack` 写 virtio 的中断应答寄存器，`gic_eoi` 结束这次中断。再敲一次 `echo hello` 不会再进来，这块还在 `bread` 的缓存里。
