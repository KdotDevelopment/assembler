global main
section .text
    main:
        mov rax, 38
        mov rcx, 2
        sub rax, rcx
        mov dil, al
        mov rax, 60
        syscall