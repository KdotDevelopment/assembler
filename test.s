global main
section .text
    main:
        ;example program - its meant to be complicated
        mov rax, 17
        mov rcx, 2
        mov rbx, 10
        imul rcx
        cmp rax, rbx 
        setg al ; is the multiplication result bigger than rbx (10)?
        movzx r8, al
        add r8, 1
        mov rax, 3
        mul r8
        mov rdi, rax ; multiply result (1 or 2) by three
        mov rax, 60
        syscall