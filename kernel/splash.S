.globl splash_start
.globl splash_end

.section .rodata.splash
.align 16
splash_start:
        .incbin "splash.raw"
splash_end:
