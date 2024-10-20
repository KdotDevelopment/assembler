# Assembler

This is an x86 assembler for my compiler (https://github.com/KdotDevelopment/c-compiler) which uses intel syntax. The assembler will output an ELF file executable on a linux machine. The overall goal for this is to be run on my custom 64-bit operating system (https://github.com/KdotDevelopment/RyanOS) along with the compiler so executable programs can be made fully within the OS.

## What can it do now?
Currently, it supports the following instructions:

mov, movzx, push, pop, ret, syscall, lea

add, or, adc, sbb, and, sub, xor, cmp,

test, not, neg, mul, imul, div, idiv,

all variations of set

all variations of jmp

call



And the following registers:

rax - rdi

r8(d/w/b) - r15(d/w/b)

eax - edi

ax - di

al - dil (and high byte registers ah, ch, dh, bh)


## What can't it do now?
The syntax for memory addressing math is a bit special and right now it cannot parse something like [rax + rcx * 2 + 4], in fact in order to use any displacement or index register, it must come outside of the brackets such as -4[rbp] or rcx[rbp], which differs from other common assemblers. -4[rbp * 2] is also allowed.