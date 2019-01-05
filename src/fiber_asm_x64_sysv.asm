
default rel

section .text

global fiber_asm_switch:function hidden
global fiber_asm_invoke:function hidden
global fiber_asm_exec_on_stack:function hidden

struc regs
regs_rsp:       resq 1
regs_rbp:       resq 1
regs_rbx:       resq 1
regs_r12:       resq 1
regs_r13:       resq 1
regs_r14:       resq 1
regs_r15:       resq 1
endstruc

fiber_asm_switch:               ; Regs *from, Regs *to -> void
        mov [rdi + regs_rbp], rbp
        mov [rdi + regs_rbx], rbx
        mov [rdi + regs_r12], r12
        mov [rdi + regs_r13], r13
        mov [rdi + regs_r14], r14
        mov [rdi + regs_r15], r15

        push rdi
        call continue
        pop rax

        mov rbp, [rax + regs_rbp]
        mov rbx, [rax + regs_rbx]
        mov r12, [rax + regs_r12]
        mov r13, [rax + regs_r13]
        mov r14, [rax + regs_r14]
        mov r15, [rax + regs_r15]

        mov rax, rsp
        add eax, 8              ; skip return address
        test eax, 0xF           ; check alignment
        jz .ok
        jmp $
.ok:
        ret
.end:
size fiber_asm_switch fiber_asm_switch.end - fiber_asm_switch

continue:
        mov [rdi + regs_rsp], rsp
        mov rsp, [rsi + regs_rsp]

        mov rax, rsp
        add eax, 8              ; skip return address
        test eax, 0xF           ; check alignment
        jz .ok
        jmp $
.ok:
        ret
.end:

fiber_asm_invoke:
        pop rsi                 ; chunk (for alignment)
        pop rsi                 ; function to call
        mov rdi, [rsp]          ; rdi points to beginning of arguments

        mov rcx, rsp
        test ecx, 0xF           ; check alignment
        jz .ok
        jmp $
.ok:
        call rsi
        mov rsp, [rsp + 8]
        ret
.end:
size fiber_asm_invoke fiber_asm_invoke.end - fiber_asm_invoke

fiber_asm_exec_on_stack:        ; stack_ptr, void (*)(void *), void * -> void
        mov rax, rsp
        lea rsp, [rdi - 8]      ; set up new stack_ptr
        mov [rsp], rax          ; and push old stack_ptr

        mov rax, rsp
        test eax, 0xF           ; check alignment
        jz .ok
        jmp $
.ok:
        mov rdi, rdx
        call rsi
        mov rsp, [rsp]          ; load old stack_ptr
        ret
.end:
size fiber_asm_exec_on_stack fiber_asm_exec_on_stack.end - fiber_asm_exec_on_stack
