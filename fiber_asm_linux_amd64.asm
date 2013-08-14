
extern asm_assert_fail
        
global fiber_asm_switch
global fiber_asm_invoke
global fiber_asm_exec_on_stack

struc regs
regs_rsp:       resq 1
regs_rbp:       resq 1
regs_rbx:       resq 1
regs_r12:       resq 1
regs_r13:       resq 1
regs_r14:       resq 1
regs_r15:       resq 1
endstruc

align 16, nop
fiber_asm_switch:               ; Regs *from, Regs *to -> void
        mov [rdi + regs_rbp], rbp
        mov [rdi + regs_rbx], rbx
        mov [rdi + regs_r12], r12
        mov [rdi + regs_r13], r13
        mov [rdi + regs_r14], r14
        mov [rdi + regs_r15], r15
        push rdi
        push fiber_asm_continue
        mov [rdi + regs_rsp], rsp
        mov rsp, [rsi + regs_rsp]

        mov rax, rsp
        add rax, 8              ; skip return address
        and rax, 15
        test rax, rax
        jz .ok
        call asm_assert_fail
.ok:     
        ret

align 16, nop
fiber_asm_continue:
        pop rax
        mov rbp, [rax + regs_rbp]
        mov rbx, [rax + regs_rbx]
        mov r12, [rax + regs_r12]
        mov r13, [rax + regs_r13]
        mov r14, [rax + regs_r14]
        mov r15, [rax + regs_r15]
        
        mov rax, rsp
        add rax, 8              ; skip return address
        and rax, 15
        test rax, rax
        jz .ok
        call asm_assert_fail
.ok:
        ret

align 16, nop
fiber_asm_invoke:
        pop rsi                 ; chunk (for alignment)
        pop rsi                 ; function to call
        mov rdi, [rsp]          ; rdi points to beginning of arguments

        mov rcx, rsp
        and rcx, 15
        test rcx, rcx
        jz .ok
        call asm_assert_fail
.ok:
        call rsi
        mov rsp, [rsp + 8]
        ret

align 16, nop
fiber_asm_exec_on_stack:        ; stack_ptr, void (*)(void *), void * -> void
        mov rax, rsp            
        lea rsp, [rdi - 8]      ; set up new stack_ptr
        mov [rsp], rax          ; and push old stack_ptr

        mov rax, rsp
        and rax, 15
        test rax, rax
        jz .ok
        call asm_assert_fail
.ok:     
        mov rdi, rdx        
        call rsi
        mov rsp, [rsp]          ; load old stack_ptr
        ret
