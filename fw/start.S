    .section .text
    .globl _start

_start:
    la sp, _stack_top
    call main
    ecall

1:
    j 1b
