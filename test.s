global main
section .text
    main:
        mov rax, 10
        mov rcx, 20
        cmp rax, rcx
        mov dil, al
        mov rax, 60
        syscall