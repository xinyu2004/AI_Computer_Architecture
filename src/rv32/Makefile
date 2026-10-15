# RV32GC integer kernel. The compiler package is gcc-riscv64-unknown-elf;
# -march=rv32gc -mabi=ilp32 selects the 32-bit multilib.
CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc

CFLAGS := -march=rv32gc -mabi=ilp32 -mcmodel=medany -mno-relax \
	-std=gnu11 -O2 -g -Wall -Wextra -ffreestanding \
	-fno-stack-protector -fno-pic -fno-builtin \
	-fno-asynchronous-unwind-tables -nostdinc

LDFLAGS := -nostdlib -static -T kernel.ld -Wl,--no-relax

OBJS := entry.o swtch.o trap.o power.o uart.o fdt.o vm.o traps.o mem.o \
	virtio.o fs.o exec.o main.o

.PHONY: all clean

USER := ../user
URV := ../riscv
.SECONDARY: $(USER)/sh.o $(USER)/ls.o $(USER)/echo.o $(USER)/sleep.o

all: kernel.elf $(USER)/sh.elf $(USER)/ls.elf $(USER)/echo.elf $(USER)/sleep.elf

kernel.elf: $(OBJS) kernel.ld
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) -lgcc

$(URV)/entry.o: $(URV)/entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

$(USER)/%.elf: $(URV)/entry.o $(USER)/%.o $(URV)/user.ld
	$(CC) $(CFLAGS) -o $@ $(URV)/entry.o $(USER)/$*.o -nostdlib -static \
		-T $(URV)/user.ld -Wl,--no-relax

$(USER)/%.o: $(USER)/%.c $(URV)/user.h
	$(CC) $(CFLAGS) -I$(URV) -c -o $@ $<

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

swtch.o: swtch.S
	$(CC) $(CFLAGS) -c -o $@ $<

trap.o: trap.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c rv32.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) kernel.elf $(URV)/entry.o \
		$(USER)/sh.o $(USER)/ls.o $(USER)/echo.o $(USER)/sleep.o \
		$(USER)/sh.elf $(USER)/ls.elf $(USER)/echo.elf $(USER)/sleep.elf
