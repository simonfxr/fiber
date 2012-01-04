
global fiber_asm_switch
global fiber_asm_push_invoker
global fiber_asm_exec_on_stack

align 16, nop
fiber_asm_switch:               ; void **, void * -> void

        mov eax, [esp+4]
        mov ecx, [esp+8]
        
        push ebp
        push esi
        push edi
        push ebx
        push fiber_asm_load_regs

        mov [eax], esp
        mov esp, ecx

        ret

align 16, nop
fiber_asm_load_regs:

        pop ebx
        pop edi
        pop esi
        pop ebp

        ret

align 16, nop
fiber_asm_invoke:

        mov edx, [esp]
        lea eax, [esp+8]
        sub esp, 8
        mov [esp], eax
        call edx

        mov eax, [esp+12]
        lea esp, [esp+eax+16]

        ret
        
align 16, nop
fiber_asm_push_invoker:         ; void ** -> void

        mov eax, [esp+4]
        
        mov ecx, [eax]
        sub ecx, 4
        mov dword [ecx], fiber_asm_invoke
        mov [eax], ecx
        
        ret

align 16, nop
fiber_asm_exec_on_stack:        ; void *, void (*)(const char *), const char * -> void

        mov eax, [esp+4]
        mov ecx, [esp+8]
        mov edx, [esp+12]

        sub eax, 16
        mov [eax+4], esp
        mov [eax], edx
        
        mov esp, eax
        call ecx
        mov esp, [esp+4]

        ret
