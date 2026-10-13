
AR	= ar
AS	= as --32 -g
LD	= ld -m elf_i386
CC	= gcc -m32
CFLAGS	= -std=gnu89 -m32 -Wall -O2 -g -fomit-frame-pointer -fno-builtin -fleading-underscore -fno-stack-protector -fno-pic -fno-pie -fcf-protection=none -march=i386 -mpreferred-stack-boundary=2 -fno-asynchronous-unwind-tables -nostdinc -I../include
CPP	= gcc -m32 -E -nostdinc -I../include

.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<
.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<

OBJS	= memory.o page.o

all: mm.o

mm.o: $(OBJS)
	$(LD) -r -o mm.o $(OBJS)

clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies:
memory.o : memory.c ../include/signal.h ../include/sys/types.h \
  ../include/asm/system.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/linux/mm.h ../include/linux/kernel.h 
