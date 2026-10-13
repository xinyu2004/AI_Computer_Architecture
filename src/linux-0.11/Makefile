#
# if you want the ram-disk device, define this to be the
# size in blocks.
#
RAMDISK = #-DRAMDISK=512

# Contemporary 32-bit toolchain. The 1991 names (as86, gas, gld) are gone,
# and the host compiler defaults to 64-bit ELF.
AS	= as --32 -g
LD	= ld -m elf_i386
LDFLAGS	= -T tools/kernel.ld
CC	= gcc -m32 $(RAMDISK)
# -g stays in the ELF for the debugger. objcopy drops it from the raw Image.
CFLAGS	= -std=gnu89 -m32 -Wall -O2 -g -fomit-frame-pointer -fno-builtin -fleading-underscore -fno-stack-protector -fno-pic -fno-pie -fcf-protection=none -march=i386 -mpreferred-stack-boundary=2 -fno-asynchronous-unwind-tables
CPP	= gcc -m32 -E -nostdinc -Iinclude
OBJCOPY	= objcopy -O binary

#
# ROOT_DEV is major,minor. 3,6 is /dev/hd6, the 1991 default.
# FLOPPY still means "use the boot floppy". A block-device path still works.
#
# 3,1 is the first partition of the first hard disk. hdc-0.11.img uses that.
ROOT_DEV = 3,1

ARCHIVES=kernel/kernel.o mm/mm.o fs/fs.o
DRIVERS =kernel/blk_drv/blk_drv.a kernel/chr_drv/chr_drv.a
MATH	=kernel/math/math.a
LIBS	=lib/lib.a

.c.s:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -S -o $*.s $<
.s.o:
	$(AS) -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-nostdinc -Iinclude -c -o $*.o $<

all:	Image

Image: boot/bootsect boot/setup tools/system tools/build
	tools/build boot/bootsect boot/setup tools/system $(ROOT_DEV) > Image
	sync

disk: Image
	dd bs=8192 if=Image of=/dev/PS0

tools/build: tools/build.c
	gcc -std=gnu89 -Wall -O2 -o tools/build tools/build.c

boot/head.o: boot/head.s

tools/system:	boot/head.o init/main.o \
		$(ARCHIVES) $(DRIVERS) $(MATH) $(LIBS)
	$(LD) $(LDFLAGS) boot/head.o init/main.o \
	$(ARCHIVES) \
	$(DRIVERS) \
	$(MATH) \
	$(LIBS) \
	-o tools/system.elf > System.map
	$(OBJCOPY) -R .note -R .comment -R .note.gnu.property \
		tools/system.elf tools/system

kernel/math/math.a:
	(cd kernel/math; make)

kernel/blk_drv/blk_drv.a:
	(cd kernel/blk_drv; make)

kernel/chr_drv/chr_drv.a:
	(cd kernel/chr_drv; make)

kernel/kernel.o:
	(cd kernel; make)

mm/mm.o:
	(cd mm; make)

fs/fs.o:
	(cd fs; make)

lib/lib.a:
	(cd lib; make)

boot/setup: boot/setup.s
	$(AS) -o boot/setup.o boot/setup.s
	$(LD) -Ttext 0 -e start -N --oformat binary -o boot/setup boot/setup.o
	$(LD) -Ttext 0 -e start -o boot/setup.elf boot/setup.o

boot/bootsect:	boot/bootsect.s
	$(AS) -o boot/bootsect.o boot/bootsect.s
	$(LD) -Ttext 0 -e start -N --oformat binary -o boot/bootsect boot/bootsect.o
	$(LD) -Ttext 0x7c00 -e start -o boot/bootsect.elf boot/bootsect.o

tmp.s:	boot/bootsect.s tools/system
	(echo -n "SYSSIZE = (";ls -l tools/system | grep system \
		| cut -c25-31 | tr '\012' ' '; echo "+ 15 ) / 16") > tmp.s
	cat boot/bootsect.s >> tmp.s

clean:
	rm -f Image System.map tmp_make core boot/bootsect boot/setup
	rm -f boot/bootsect.elf boot/setup.elf
	rm -f init/*.o tools/system tools/system.elf tools/build boot/*.o
	(cd mm;make clean)
	(cd fs;make clean)
	(cd kernel;make clean)
	(cd lib;make clean)

backup: clean
	(cd .. ; tar cf - linux | compress - > backup.Z)
	sync

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in init/*.c;do echo -n "init/";$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
	(cd fs; make dep)
	(cd kernel; make dep)
	(cd mm; make dep)

### Dependencies:
init/main.o : init/main.c include/unistd.h include/sys/stat.h \
  include/sys/types.h include/sys/times.h include/sys/utsname.h \
  include/utime.h include/time.h include/linux/tty.h include/termios.h \
  include/linux/sched.h include/linux/head.h include/linux/fs.h \
  include/linux/mm.h include/signal.h include/asm/system.h include/asm/io.h \
  include/stddef.h include/stdarg.h include/fcntl.h 
