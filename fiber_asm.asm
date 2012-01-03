
global fiber_asm_switch
global fiber_asm_push_thunk
global fiber_asm_exec_on_stack

        ;; FIXME: stack has to be 16 byte aligned whenever a external
        ;; function is called

fiber_asm_switch:               ; void **, void * -> void
        push rbx
        push rbp
        push r12
        push r13
        push r14
        push r15
        push fiber_asm_load_regs
        mov [rdi], rsp
        mov rsp, rsi
        ret

fiber_asm_load_regs:
        pop r15
        pop r14
        pop r13
        pop r12
        pop rbp
        pop rbx
        ret

fiber_asm_invoke:
        pop rax
        pop rdi
        pop rdx
        mov rcx, rsp
        sub rcx, rdx
        sub rcx, 7
        and rcx, -8
        sub rsp, 8
        lea [rsp], [rcx+8]
        call rax
        pop rsp
        ret

fiber_asm_push_thunk:           ; void **, void (*)(...), Fiber *, const char *, size_t -> void
        mov rax, [rdi]
        xor r9, r9
        
        test r8, r8
        jz .done_copy
.copy:
        mov byte ptr [rax] [rcx+r9]
        add r9, 1
        sub rax, 1
        cmp r9, r8
        jne .copy
        
.done_copy:
        
        sub rax, 7
        and rax, -8             ; 8 byte align
        
        mov [rax], r8
        mov [rax-8], rdx
        mov [rax-16], rsi
        mov [rax-24], fiber_asm_invoke
        sub rax, 24
        mov [rdi], rax
        ret
        
fiber_asm_exec_on_stack:        ; void *, void (*)(const char *, size_t), const char *, size_t -> void
        mov rax, rsp
        mov rsp, rdi
        push rax
        mov rax, rsi
        mov rdi, rdx
        mov rsi, rcx
        call rax
        pop rsp
        ret
