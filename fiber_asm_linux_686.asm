
global fiber_asm_switch
global fiber_asm_push_invoker
global fiber_asm_exec_on_stack

align 16, nop
fiber_asm_switch:               ; void **, void * -> void
        ret

align 16, nop
fiber_asm_push_invoker:         ; void ** -> void
        ret

align 16, nop
fiber_asm_exec_on_stack:        ; void *, void (*)(const char *), const char * -> void
        ret
