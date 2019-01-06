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

  mov [edx + regs_esp], esp
  mov [edx + regs_ebp], ebp
  mov [edx + regs_ebx], ebx
  mov [edx + regs_edi], edi
  mov [edx + regs_esi], esi

  mov esp, [ecx + regs_esp]
  mov ebp, [ecx + regs_ebp]
  mov ebx, [ecx + regs_ebx]
  mov edi, [ecx + regs_edi]
  mov esi, [ecx + regs_esi]
  ret
.end:
  size fiber_asm_switch fiber_asm_switch.end - fiber_asm_switch

fiber_asm_invoke:
  pop edx
  pop edx
  call edx
  mov esp, [esp + 4]
  ret

.end:
size fiber_asm_invoke fiber_asm_invoke.end - fiber_asm_invoke

fiber_asm_exec_on_stack:        ; stack_ptr, void (*)(void *), void * -> void
  mov eax, esp
  mov esp, [eax + 4]
  mov ecx, [eax + 8]
  mov edx, [eax + 12]
  push eax
  sub esp, 4
  push edx
  call ecx
  add esp, 8
  pop esp
  ret

.end:
size fiber_asm_exec_on_stack fiber_asm_exec_on_stack.end - fiber_asm_exec_on_stack
