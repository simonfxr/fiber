  .intel_syntax noprefix
  .text

  .global fiber_asm_switch
  .def    fiber_asm_switch
  .scl    3
  .type   32
  .endef
fiber_asm_switch:           // Regs *from, Regs *to -> void
  jmp fiber_align_check_failed

  .global fiber_asm_invoke
  .def    fiber_asm_invoke
  .scl    3
  .type   32
  .endef
fiber_asm_invoke:
  jmp fiber_align_check_failed

  .global fiber_asm_exec_on_stack
  .def    fiber_asm_exec_on_stack
  .scl    3
  .type   32
  .endef
fiber_asm_exec_on_stack:  // stack_ptr, void (*)(void *), void * -> void
  jmp fiber_align_check_failed
