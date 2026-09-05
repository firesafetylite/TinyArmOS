ENTRY(NativeMain)
SECTIONS {
    . = 0x41000000;
    .text : { *(.text.NativeMain) *(.text*) }
    .rodata : ALIGN(16) { KEEP(*(.native_abi)) *(.rodata*) }
    .data : ALIGN(16) { *(.data*) }
    .bss (NOLOAD) : ALIGN(16) { *(.bss*) *(COMMON) }
    /DISCARD/ : { *(.comment*) *(.eh_frame*) *(.note*) }
}
ASSERT(. < 0x44000000, "Native OS exceeds its load window")
