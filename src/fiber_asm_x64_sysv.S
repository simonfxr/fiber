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

fiber_asm_switch:           // Regs *from, Regs *to -> void
  mov [rdi + 0], rsp
  mov [rdi + 8], rbp
  mov [rdi + 16], rbx
  mov [rdi + 24], r12
  mov [rdi + 32], r13
  mov [rdi + 40], r14
  mov [rdi + 48], r15

  mov rsp, [rsi + 0]
  mov rbp, [rsi + 8]
  mov rbx, [rsi + 16]
  mov r12, [rsi + 24]
  mov r13, [rsi + 32]
  mov r14, [rsi + 40]
  mov r15, [rsi + 48]

  ret
  .size fiber_asm_switch, .-fiber_asm_switch

fiber_asm_invoke:
  add rsp, 8              // skip filler for alignment
  pop rsi                 // function to call
  mov rdi, [rsp]          // rdi points to beginning of arguments

#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif
  call rsi
  mov rsp, [rsp + 8]
  ret
  .size fiber_asm_invoke, .-fiber_asm_invoke

fiber_asm_exec_on_stack:  // stack_ptr, void (*)(void *), void * -> void
  mov rax, rsp
  mov rsp, rdi
  push rax
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jz 1f
  jmp fiber_align_check_failed
1:
#endif
  mov rdi, rdx
  call rsi
  mov rsp, [rsp]          // load old stack_ptr
  ret
  .size fiber_asm_exec_on_stack, .-fiber_asm_exec_on_stack
