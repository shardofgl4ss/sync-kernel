/* OUTPUT_FORMAT(binary) */
ENTRY(start)

SECTIONS
{
        . = 0x10000;
        kstack_top = .;
        _begin = .;

        .text ALIGN(0x1000) :
        {
                *(.text.stub)
                *(.text)
                *(.text.isr)
        }

        .rodata ALIGN(0x1000) :
        {
                *(.rodata*)
        }

        .data ALIGN(0x1000) :
        {
                __data_start = .;
                *(.data)
                *(.data.*)
                __data_end = .;
        }

        .bss ALIGN(0x1000) : {
                __bss_start = .;
                *(COMMON)
                *(.bss)
                __bss_end = .;
        }

	.pt ALIGN(0x1000) : {
		__PAGETABLE_START = .;
		*(.pt)
		__PAGETABLE_END = .;
	}

        /DISCARD/ :
        {
                *(.eh_frame)
                *(.comment)
                *(.note.gnu.property)
                *(.note*)
        }

        . = ALIGN(0x1000);
        _end = .;

        .vga 0xb8000 (NOLOAD) : {
                _vga_start = .;
                *(.vga)
                _vga_end = .;
        }
}
