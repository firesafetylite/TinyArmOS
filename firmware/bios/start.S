/* QEMU virt reset vector. Code executes from flash; writable state lives in RAM. */
.section .text.boot,"ax"
.global _start
_start:
    msr daifset, #15
    mrs x0, mpidr_el1
    and x0, x0, #0xffffff
    cbnz x0, park
    ldr x0, =__stack_top
    mov sp, x0
    ldr x0, =__data_load
    ldr x1, =__data_start
    ldr x2, =__data_end
1:  cmp x1, x2
    b.hs 2f
    ldrb w3, [x0], #1
    strb w3, [x1], #1
    b 1b
2:  ldr x1, =__bss_start
    ldr x2, =__bss_end
3:  cmp x1, x2
    b.hs 4f
    strb wzr, [x1], #1
    b 3b
4:  ldr x0, =vectors
    mrs x1, CurrentEL
    cmp x1, #8
    b.eq 5f
    cmp x1, #4
    b.ne park
    msr vbar_el1, x0
    b 6f
5:  msr vbar_el2, x0
    mrs x0, cptr_el2
    bic x0, x0, #(1 << 10)
    bic x0, x0, #(1 << 31)
    msr cptr_el2, x0
6:  mrs x0, cpacr_el1
    orr x0, x0, #(3 << 20)
    msr cpacr_el1, x0
    isb
    bl bios_main
park:
    wfe
    b park

.balign 2048
vectors:
.rept 16
    b exception
    .balign 128
.endr
exception:
    mrs x0, CurrentEL
    cmp x0, #8
    b.eq 7f
    mrs x1, esr_el1
    mrs x2, elr_el1
    b 8f
7:  mrs x1, esr_el2
    mrs x2, elr_el2
8:  bl bios_exception
    b park
