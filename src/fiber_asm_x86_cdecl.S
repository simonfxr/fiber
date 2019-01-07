  .intel_syntax noprefix
  .text

  .global fiber_asm_switch
  .hidden fiber_asm_switch
  .type   fiber_asm_switch, @function

  .global fiber_asm_invoke
  .hidden fiber_asm_invoke
  .type   fiber_asm_invoke, @function

  .global fiber_asm_exec_on_stack
  .hidden fiber_asm_exec_on_stack
  .type   fiber_asm_invoke, @function

fiber_asm_switch:
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
  .size fiber_asm_switch, .-fiber_asm_switch

fiber_asm_invoke:
  mov edx, [esp + 4]
  mov eax, [esp + 8]
  mov [esp], eax
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif
  call edx
  mov esp, [esp + 12]
  ret
  .size fiber_asm_invoke, .-fiber_asm_invoke

fiber_asm_exec_on_stack:
  mov eax, esp
  mov esp, [eax + 4]
  mov ecx, [eax + 8]
  mov edx, [eax + 12]
  push eax
  sub esp, 4
  push edx
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif
  call ecx
  add esp, 8
  pop esp
  ret
  .size fiber_asm_exec_on_stack, .-fiber_asm_exec_on_stack
