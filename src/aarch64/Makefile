# AArch64 kernel for Cortex-A57. User programs are built here so they
# do not replace the ARMv7 or RISC-V ELF files.
# -mgeneral-regs-only keeps the kernel off the FP registers.
# -mstrict-align: with the MMU off, memory is Device and an unaligned
# store faults. kernel.bin is the Image QEMU loads. kernel.elf is for
# the debugger. --nmagic keeps a user ELF from being padded out to its
# link address.
CROSS ?= aarch64-linux-gnu-
CC := $(CROSS)gcc

CFLAGS := -mcpu=cortex-a57 -mgeneral-regs-only -mstrict-align \
	-std=gnu11 -O2 -g -Wall -Wextra -ffreestanding \
	-fno-stack-protector -fno-pic -fno-builtin \
	-fno-asynchronous-unwind-tables -nostdinc

LDFLAGS := -nostdlib -static -T kernel.ld

OBJS := entry.o swtch.o trap.o power.o uart.o fdt.o vm.o traps.o mem.o \
	gic.o virtio.o fs.o exec.o main.o

.PHONY: all clean

USER := ../user
UDIR := user
.SECONDARY: $(UDIR)/sh.o $(UDIR)/ls.o $(UDIR)/echo.o $(UDIR)/sleep.o

all: kernel.elf kernel.bin $(UDIR)/sh.elf $(UDIR)/ls.elf $(UDIR)/echo.elf $(UDIR)/sleep.elf

kernel.elf: $(OBJS) kernel.ld
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) -lgcc

kernel.bin: kernel.elf
	$(CROSS)objcopy -O binary $< $@

$(UDIR)/entry.o: user_entry.S
	mkdir -p $(UDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UDIR)/%.elf: $(UDIR)/entry.o $(UDIR)/%.o user.ld
	$(CC) $(CFLAGS) -o $@ $(UDIR)/entry.o $(UDIR)/$*.o -nostdlib -static \
		-T user.ld -Wl,--nmagic -lgcc

$(UDIR)/%.o: $(USER)/%.c user.h
	mkdir -p $(UDIR)
	$(CC) $(CFLAGS) -I. -c -o $@ $<

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

swtch.o: swtch.S
	$(CC) $(CFLAGS) -c -o $@ $<

trap.o: trap.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c aarch64.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) kernel.elf kernel.bin \
		$(UDIR)/entry.o $(UDIR)/sh.o $(UDIR)/ls.o $(UDIR)/echo.o $(UDIR)/sleep.o \
		$(UDIR)/sh.elf $(UDIR)/ls.elf $(UDIR)/echo.elf $(UDIR)/sleep.elf
