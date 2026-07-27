OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(start)

SECTIONS
{
        . = 0x0000;
        _begin = .;

        .text :
        {
                *(.text)
        }

        .rodata :
        {
                *(.rodata*)
        }

        .data :
        {
                __data_start = .;
                *(.data)
                *(.data.*)
                __data_end = .;
        }

        .bss : {
                __bss_start = .;
                *(COMMON)
                *(.bss)
                __bss_end = .;
        }

        /DISCARD/ :
        {
                *(.eh_frame)
                *(.comment)
                *(.note.gnu.property)
                *(.note*)
        }

        _end = .;
}
