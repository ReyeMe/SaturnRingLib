/* FLD kernel image: runs at 0x00200000, word 0 is the entry pointer the IP code calls through. Everything,
   variables included, is in the binary, as nothing clears memory before the kernel runs. */
OUTPUT_FORMAT("elf32-sh")
ENTRY(_FLD_1stRead)

SECTIONS
{
    . = 0x00200000;

    .image :
    {
        LONG(_FLD_1stRead)
        *(.text .text.*)
        *(.rodata .rodata.*)
        *(.data .data.*)
        *(.bss .bss.* COMMON)
        . = ALIGN(4);
    }

    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame) }
}
