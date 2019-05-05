  .altmacro
  .arch armv8-a
  .text

#define FUNC(sym) .align 4; .global sym; .hidden sym; .type sym, @function; sym
#define END_FUNC(sym) .size sym, .-sym

  .macro check_stack_alignment_nomove reg
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    tst \reg, 0xF
    bne align_check_failed
#endif
  .endm

  .macro check_stack_alignment_move reg
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    mov \reg, sp
    check_stack_alignment_nomove \reg
#endif
  .endm

FUNC(fiber_asm_switch):
  mov x3, sp
  str x3, [x0], 8
  ldr x3, [x1], 8
  mov sp, x3

  stp x30, x29, [x0], 16
  ldp x30, x29, [x1], 16
  stp x19, x20, [x0], 16
  ldp x19, x20, [x1], 16
  stp x21, x22, [x0], 16
  ldp x21, x22, [x1], 16
  stp x23, x24, [x0], 16
  ldp x23, x24, [x1], 16
  stp x25, x26, [x0], 16
  ldp x25, x26, [x1], 16
  stp x27, x28, [x0], 16
  ldp x27, x28, [x1], 16

  stp d8, d9, [x0], 16
  ldp d8, d9, [x1], 16
  stp d10, d11, [x0], 16
  ldp d10, d11, [x1], 16
  stp d12, d13, [x0], 16
  ldp d12, d13, [x1], 16
  stp d14, d15, [x0], 16
  ldp d14, d15, [x1], 16

  check_stack_alignment_nomove x3
  ret
END_FUNC(fiber_asm_switch)

FUNC(fiber_asm_invoke):
  ldp x1, x0, [sp], 16
  check_stack_alignment_move x2
  blr x1
  ldp x30, x1, [sp], 16
  mov sp, x1
  check_stack_alignment_nomove x1
  ret
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack):
  mov x3, sp
  stp x3, x30, [x0, -16]!
  mov sp, x0
  check_stack_alignment_nomove x0
  mov x0, x2
  blr x1
  ldp x3, x30, [sp]
  mov sp, x3
  ret
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  b fiber_align_check_failed
#endif
