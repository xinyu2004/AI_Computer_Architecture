# RV32GC integer kernel. The compiler package is gcc-riscv64-unknown-elf;
# -march=rv32gc -mabi=ilp32 selects the 32-bit multilib.
CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc

CFLAGS := -march=rv32gc -mabi=ilp32 -mcmodel=medany -mno-relax \
	-std=gnu11 -O2 -g -Wall -Wextra -ffreestanding \
	-fno-stack-protector -fno-pic -fno-builtin \
	-fno-asynchronous-unwind-tables -nostdinc

LDFLAGS := -nostdlib -static -T kernel.ld -Wl,--no-relax

OBJS := entry.o swtch.o trap.o sbi.o uart.o fdt.o vm.o traps.o mem.o main.o

.PHONY: all clean

all: kernel.elf

kernel.elf: $(OBJS) kernel.ld
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) -lgcc

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

swtch.o: swtch.S
	$(CC) $(CFLAGS) -c -o $@ $<

trap.o: trap.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c rv32.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) kernel.elf
