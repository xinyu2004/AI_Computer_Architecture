# Sourced by .vscode/launch.json after it has connected to QEMU.
# Hardware breakpoints are unavailable before that connection.
# The editor continues after this file; do not continue here.
# The editor loads boot/bootsect.elf. Its first instruction runs at 0x7C00
# with CS = 0, so the source line matches. A hardware breakpoint survives
# the BIOS writing the boot sector into RAM.
#
# After ljmp $INITSEG, $go the CPU uses CS = 0x9000. GDB then reports the
# offset alone, so those later bootsect.s lines are not matched. setup.s
# is loaded at 0x90200. tools/system.elf stays unloaded: it covers
# 0x7C00 and would hide the boot sector.

set architecture i8086
set disassembly-flavor att
set confirm off
set pagination off

add-symbol-file src/linux-0.11/boot/setup.elf 0x90200

# One hardware breakpoint, with no commands. A command list here runs
# when the first instruction hits; silent commands leave the editor in
# the Running state, so step and registers never become available.
hbreak *0x7c00
