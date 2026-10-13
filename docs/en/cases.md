# Walk through the chapters

[中文](../zh/cases.md)

These five chapters split steps 3 and 4. Each chapter asks one thing: read a piece of the 1991 source, stop with a launcher that already exists, and look at machine state or at the cost of execution. There is no new launcher.

Addresses move every time the kernel is linked. Use the address `labs/bochs/launch.sh` or `labs/gem5/launch.sh` prints for the build you just made. The hex numbers in the chapters are from one link of this tree.

1. [Protected mode](cases/protected-mode.md). `lmsw` sets CR0.PE, and the GDT holds the flat code and data segments.
2. [Filling the page tables](cases/paging.md). `setup_paging` writes the tables for the first 16MB. Predict the cache misses, then read the gem5 counters.
3. [The timer interrupt](cases/timer.md). IRQ0 arrives as vector `0x20` and enters `_timer_interrupt`.
4. [The task switch](cases/schedule.md). `_schedule` uses `ljmp` to switch to task 1's TSS.
5. [Copying a page](cases/copy-page.md). The `rep movsl` in `copy_page` copies 4KiB and fills the store queue.

Press F5 in QEMU to single-step the source. Bochs shows the descriptor tables and the task state. gem5 prints cycles, cache misses, and pipeline stalls.
