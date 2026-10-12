OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(startup_32)
SECTIONS
{
  . = 0x0;
  .text : {
    boot/head.o(.text)
    *(.text)
    *(.text.*)
  }
  PROVIDE(_etext = .);
  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }
  .data : {
    *(.data)
    *(.data.*)
  }
  PROVIDE(_edata = .);
  .bss : {
    *(.bss)
    *(.bss.*)
    *(COMMON)
  }
  PROVIDE(_end = .);
  /DISCARD/ : {
    *(.eh_frame)
    *(.eh_frame_hdr)
    *(.note*)
    *(.comment)
    *(.gnu*)
  }
}
