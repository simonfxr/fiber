.MODEL flat, C
.CODE

PUBLIC fiber_asm_switch
PUBLIC fiber_asm_invoke
PUBLIC fiber_asm_exec_on_stack

fiber_asm_switch PROC
  mov edx, [esp+4]
  mov ecx, [esp+8]
  mov [edx+0], esp
  mov esp, [ecx+0]
  mov [edx+4], ebp
  mov ebp, [ecx+4]
  mov [edx+8], ebx
  mov ebx, [ecx+8]
  mov [edx+12], edi
  mov edi, [ecx+12]
  mov [edx+16], esi
  mov esi, [ecx+16]
  ret
fiber_asm_switch ENDP

fiber_asm_invoke PROC
  call [esp+4]
  mov esp, [esp+8]
  ret
fiber_asm_invoke ENDP

fiber_asm_exec_on_stack PROC
  mov eax, esp
  mov edx, [eax+4]
  mov ecx, [eax+8]
  mov esp, [eax+12]
  sub esp, 12
  mov [esp+4], eax
  mov [esp], edx
  call ecx
  mov esp, [esp+4]
  ret
fiber_asm_exec_on_stack ENDP

END
