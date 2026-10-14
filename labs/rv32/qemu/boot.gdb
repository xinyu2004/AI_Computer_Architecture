# Sourced by .vscode/launch.json after it has connected to QEMU.
# QEMU is paused at its reset vector. The kernel is already loaded at
# 0x80000000. This breakpoint is _start in entry.S, the first instruction.
# The editor continues after this file; do not continue here.
# The editor reports this hit as the entry stop. launch.json keeps
# stopAtEntry true so the session stays here. With that off, the editor
# continues and the kernel runs to shutdown.

set architecture riscv:rv32
set confirm off
set pagination off

hbreak _start
