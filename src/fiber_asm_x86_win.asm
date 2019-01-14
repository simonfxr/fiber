.MODEL flat, C
.CODE

IFDEF FIBER_ASM_CHECK_ALIGNMENT
fiber_align_check_failed PROTO C
ENDIF

PUBLIC fiber_asm_switch
PUBLIC fiber_asm_invoke
PUBLIC fiber_asm_exec_on_stack

fiber_asm_switch PROC
  mov edx, [esp + 4]
  mov ecx, [esp + 8]

  mov [edx + 0], esp
  mov [edx + 4], ebp
  mov [edx + 8], ebx
  mov [edx + 12], edi
  mov [edx + 16], esi

  mov esp, [ecx + 0]
  mov ebp, [ecx + 4]
  mov ebx, [ecx + 8]
  mov edi, [ecx + 12]
  mov esi, [ecx + 16]

  ret
fiber_asm_switch ENDP


fiber_asm_invoke PROC
  mov edx, [esp + 4]
  mov eax, [esp + 8]
  mov [esp], eax

IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 3
  jnz fiber_align_check_failed
ENDIF
  call edx
  mov esp, [esp + 12]
  ret
fiber_asm_invoke ENDP


fiber_asm_exec_on_stack PROC
  mov eax, esp
  mov esp, [eax + 4]
  mov ecx, [eax + 8]
  mov edx, [eax + 12]
  push eax
  sub esp, 4
  push edx
IFDEF FIBER_ASM_CHECK_ALIGNMENT
  test esp, 3
  jnz fiber_align_check_failed
ENDIF
  call ecx
  add esp, 8
  pop esp
  ret
fiber_asm_exec_on_stack ENDP

END
