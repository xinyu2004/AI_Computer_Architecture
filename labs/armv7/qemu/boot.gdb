# Sourced by .vscode/launch.json after it has connected to QEMU.
# QEMU is paused in its boot stub. The raw image is already loaded at
# 0x40010000. This breakpoint is _start in entry.S. The stub jumps there.
# The editor continues after this file; do not continue here.
# The editor reports this hit as the entry stop. launch.json keeps
# stopAtEntry true so the session stays here. With that off, the editor
# continues and the kernel runs to shutdown.

set architecture arm
set confirm off
set pagination off

hbreak _start
