MBALIGN     equ  1 << 0
MEMINFO     equ  1 << 1
VIDEOMODE   equ  1 << 2
FLAGS       equ  MBALIGN | MEMINFO | VIDEOMODE
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0
    dd 1024
    dd 768
    dd 32

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kmain

_start:
    cli
    mov esp, stack_top
    push ebx
    push eax
    call kmain

.halt:
    hlt
    jmp .halt
