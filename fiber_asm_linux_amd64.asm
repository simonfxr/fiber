
global fiber_asm_switch
global fiber_asm_push_invoker
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
        push .load_regs
        mov [rdi], rsp
        mov rsp, rsi
        ret

.load_regs:
        pop r15
        pop r14
        pop r13
        pop r12
        pop rbp
        pop rbx
        ret

fiber_asm_invoke:
        pop rax
        pop rsi
        mov rdi, rsp
        sub rsp, 16
        add rax, rdi
        mov [rsp], rax
        call rsi
        mov rsp, [rsp]
        ret
        
fiber_asm_push_invoker:           ; void ** -> void
        mov rax, [rdi]
        sub rax, 8
        mov qword [rax], fiber_asm_invoke
        mov [rdi], rax
        ret

fiber_asm_exec_on_stack:        ; void *, void (*)(const char *), const char * -> void
        mov rax, rsp
        lea rsp, [rdi - 16]
        mov [rsp], rax
        mov rdi, rdx
        call rsi
        mov rsp, [rsp]
        ret
