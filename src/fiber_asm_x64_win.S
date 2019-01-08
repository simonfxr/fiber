  .intel_syntax noprefix
  .text

  .global fiber_asm_switch
  .def    fiber_asm_switch
  .scl    3
  .type   32
  .endef
fiber_asm_switch:           // Regs *from, Regs *to -> void
  mov [rcx + 0], rsp
  mov [rcx + 8], rbx
  mov [rcx + 16], rbp
  mov [rcx + 24], rdi
  mov [rcx + 32], rsi
  mov [rcx + 40], r12
  mov [rcx + 48], r13
  mov [rcx + 56], r14
  mov [rcx + 64], r15

  mov rsp, [rdx + 0]
  mov rbx, [rdx + 8]
  mov rbp, [rdx + 16]
  mov rdi, [rdx + 24]
  mov rsi, [rdx + 32]
  mov r12, [rdx + 40]
  mov r13, [rdx + 48]
  mov r14, [rdx + 56]
  mov r15, [rdx + 64]

  ret


  .global fiber_asm_invoke
  .def    fiber_asm_invoke
  .scl    3
  .type   32
  .endef
fiber_asm_invoke:
  add rsp, 8
  pop rdx
  mov rcx, [rsp]
  sub rsp, 32             // shadow space

#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif

  call rdx
  mov rsp, [rsp + 40]
  ret


  .global fiber_asm_exec_on_stack
  .def    fiber_asm_exec_on_stack
  .scl    3
  .type   32
  .endef
fiber_asm_exec_on_stack:  // stack_ptr, void (*)(void *), void * -> void
  mov rax, rsp
  mov rsp, rcx
  push rax
  sub rsp, 32             // shadow space

#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif
  mov rcx, r8
  call rdx
  mov rsp, [rsp + 32]
  ret
