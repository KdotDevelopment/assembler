global main
section .text
    main:
        mov rcx, 30
        push rcx
        pop rbx
        mov rdi, rbx
        mov rax, 60
        syscall
