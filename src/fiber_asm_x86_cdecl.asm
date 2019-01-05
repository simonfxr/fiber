section .text

global fiber_asm_switch:function hidden
global fiber_asm_invoke:function hidden
global fiber_asm_exec_on_stack:function hidden

struc regs
regs_esp:       resd 1
regs_ebp:       resd 1
regs_ebx:       resd 1
regs_edi:       resd 1
regs_esi:       resd 1
endstruc

fiber_asm_switch:       ; Regs *from, Regs *to -> void
        mov edx, [esp + 4]      ; from
        mov ecx, [esp + 8]      ; to

        mov [edx + regs_ebp], ebp
        mov [edx + regs_ebx], ebx
        mov [edx + regs_edi], edi
        mov [edx + regs_esi], esi

        call continue
        mov edx, [esp + 4]
        mov ebp, [edx + regs_ebp]
        mov ebx, [edx + regs_ebx]
        mov edi, [edx + regs_edi]
        mov esi, [edx + regs_esi]
        ret
.end:
size fiber_asm_switch fiber_asm_switch.end - fiber_asm_switch

continue:
        mov [edx + regs_esp], esp
        mov esp, [ecx + regs_esp]
        ret

fiber_asm_invoke:
        pop edx
        call edx
        mov esp, [esp + 4]
        ret

.end:
size fiber_asm_invoke fiber_asm_invoke.end - fiber_asm_invoke

fiber_asm_exec_on_stack:        ; stack_ptr, void (*)(void *), void * -> void
        mov edx, [esp + 4]
        mov ecx, [esp + 8]
        mov eax, [esp + 12]
        push ebx
        mov ebx, esp
        mov esp, edx
        push eax
        call ecx
        mov esp, ebx
        pop ebx
        ret

.end:
size fiber_asm_exec_on_stack fiber_asm_exec_on_stack.end - fiber_asm_exec_on_stack
