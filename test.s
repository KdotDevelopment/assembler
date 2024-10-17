global main
section .text
    main:
        mov rax, 20
        mov rbx, 20
        cmp rax, rbx
        setnz al
        mov rdi, rax
        mov rax, 60
        syscall