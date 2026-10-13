# Protected mode

[中文](../../zh/cases/protected-mode.md)

[Chapters](../cases.md)

## What this chapter is for

The CPU leaves real mode when CR0.PE is set and a GDT is in place. `lmsw` sets PE. The far jump after it loads selector 8. The code and data segments both have base 0.

## What to read

`lmsw %ax` at line 129 of `src/linux-0.11/boot/setup.s`, and the `ljmp $8,$0` that follows it. The `lgdt` in the same file loads a temporary table.

## How to start

From the repository root:

```bash
make -C src/linux-0.11
labs/bochs/launch.sh
```

Type the commands in that terminal. The VGA window is the console. The script prints the linear address of `lmsw`. On this link the example is `0x90301`. Use the address the script printed:

```text
pb 0x90301
c
```

The CPU stops on `lmsw` with PE still clear. Step twice. The next instruction is the far jump:

```text
s
s
creg
sreg
info gdt 0 2
```

The machine, VNC, and the disk lock are in [Bochs](../bochs.md).

## What you should see

`creg` shows PE set. In `sreg`, CS is selector `0x0008`: a 32-bit code segment, base `0`, limit `0x7fffff`. In `info gdt 0 2`, entry 0 is the null descriptor, selector 8 is the code segment, and selector `0x10` is the data segment, with the same base and the same limit. This is the temporary GDT from `setup.s`. The kernel loads its own GDT later, in `boot/head.s`.
