ENTRY(_start)
MEMORY {
    FLASH (rx) : ORIGIN = 0, LENGTH = 64M
    RAM (rw) : ORIGIN = 0x40100000, LENGTH = 3M
}
SECTIONS {
    .text : { KEEP(*(.text.boot)) *(.text*) } > FLASH
    .rodata : { *(.rodata*) } > FLASH
    .data : ALIGN(16) {
        __data_start = .; *(.data*) __data_end = .;
    } > RAM AT> FLASH
    __data_load = LOADADDR(.data);
    .bss (NOLOAD) : ALIGN(4096) {
        __bss_start = .; *(.bss*) *(COMMON) __bss_end = .;
    } > RAM
    __stack_top = 0x40800000;
    /DISCARD/ : { *(.comment*) *(.eh_frame*) *(.note*) }
}
ASSERT(__bss_end <= 0x40400000, "Firmware data exceeds reserved RAM")
