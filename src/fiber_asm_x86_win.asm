.MODEL flat, C
.CODE

fiber_align_check_failed PROTO C

PUBLIC fiber_asm_switch
PUBLIC fiber_asm_invoke
PUBLIC fiber_asm_exec_on_stack

fiber_asm_switch PROC
  jmp fiber_align_check_failed
fiber_asm_switch ENDP


fiber_asm_invoke PROC
  jmp fiber_align_check_failed
fiber_asm_invoke ENDP


fiber_asm_exec_on_stack PROC
  jmp fiber_align_check_failed
fiber_asm_exec_on_stack ENDP

END
