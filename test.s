global main
section .text
    main:
        mov r8, 9
        add r8, 10
        mov rdi, r8
        mov rax, 60
        syscall