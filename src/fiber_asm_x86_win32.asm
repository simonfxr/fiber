.MODEL flat, C
.CODE

PUBLIC fiber_asm_switch
PUBLIC fiber_asm_invoke
PUBLIC fiber_asm_exec_on_stack

fiber_asm_switch PROC
  pop eax
  mov edx, [esp]
  mov ecx, [esp+4]
  mov [edx], esp
  mov esp, [ecx]
  mov [edx+4], eax
  mov eax, [ecx+4]
  mov [edx+8], ebp
  mov ebp, [ecx+8]
  mov [edx+12], ebx
  mov ebx, [ecx+12]
  mov [edx+16], edi
  mov edi, [ecx+16]
  mov [edx+20], esi
  mov esi, [ecx+20]
  jmp eax
fiber_asm_switch ENDP

fiber_asm_invoke PROC
  mov eax, [esp+4]
  call eax
  mov eax, [esp+12]
  mov esp, [esp+8]
  jmp eax
fiber_asm_invoke ENDP

fiber_asm_exec_on_stack PROC
  mov eax, esp
  mov edx, [eax+4]
  mov ecx, [eax+8]
  mov esp, [eax+12]
  sub esp, 16
  mov [esp+4], eax
  mov [esp], edx
  call ecx
  mov esp, [esp+4]
  ret
fiber_asm_exec_on_stack ENDP

END
