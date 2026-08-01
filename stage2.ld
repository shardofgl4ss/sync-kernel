/*OUTPUT_FORMAT(binary)*/
ENTRY(start)

SECTIONS
{
        . = 0x10000;
        _begin = .;

        .text :
        {
                KEEP(*(.text.stub))
                KEEP(*(.rodata.stub))
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
