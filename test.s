global main
section .text
    main:
        mov rbp, rsp
        sub rsp, 64
        mov eax, 4
        mov ecx, 10
        lea r8, rcx[rax * 4]
        mov -4[rbp], 4
        mov -8[rbp], r8
        mov r14, -8[rbp]
        mov rdi, r14
        mov rax, 60
        syscall
