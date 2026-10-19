# RV64GC integer kernel. -mabi=lp64 is the 64-bit ABI. User programs are
# built here so they do not replace the RV32 ELF files in src/user/.
CROSS ?= riscv64-unknown-elf-
CC := $(CROSS)gcc

CFLAGS := -march=rv64gc -mabi=lp64 -mcmodel=medany -mno-relax \
	-std=gnu11 -O2 -g -Wall -Wextra -ffreestanding \
	-fno-stack-protector -fno-pic -fno-builtin \
	-fno-asynchronous-unwind-tables -nostdinc

LDFLAGS := -nostdlib -static -T kernel.ld -Wl,--no-relax

OBJS := entry.o swtch.o trap.o power.o uart.o fdt.o vm.o traps.o mem.o \
	virtio.o fs.o exec.o main.o

.PHONY: all clean

USER := ../user
URV := ../riscv
UDIR := user
.SECONDARY: $(UDIR)/sh.o $(UDIR)/ls.o $(UDIR)/echo.o $(UDIR)/sleep.o

all: kernel.elf $(UDIR)/sh.elf $(UDIR)/ls.elf $(UDIR)/echo.elf $(UDIR)/sleep.elf

kernel.elf: $(OBJS) kernel.ld
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS) -lgcc

$(UDIR)/entry.o: $(URV)/entry.S
	mkdir -p $(UDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(UDIR)/%.elf: $(UDIR)/entry.o $(UDIR)/%.o $(URV)/user.ld
	$(CC) $(CFLAGS) -o $@ $(UDIR)/entry.o $(UDIR)/$*.o -nostdlib -static \
		-T $(URV)/user.ld -Wl,--no-relax

$(UDIR)/%.o: $(USER)/%.c $(URV)/user.h
	mkdir -p $(UDIR)
	$(CC) $(CFLAGS) -I$(URV) -c -o $@ $<

entry.o: entry.S
	$(CC) $(CFLAGS) -c -o $@ $<

swtch.o: swtch.S
	$(CC) $(CFLAGS) -c -o $@ $<

trap.o: trap.S
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c rv64.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) kernel.elf \
		$(UDIR)/entry.o $(UDIR)/sh.o $(UDIR)/ls.o $(UDIR)/echo.o $(UDIR)/sleep.o \
		$(UDIR)/sh.elf $(UDIR)/ls.elf $(UDIR)/echo.elf $(UDIR)/sleep.elf
