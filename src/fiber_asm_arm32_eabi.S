  .arch armv6
  .syntax unified
  .arm
  .fpu vfp
  .text

#define FUNC(sym) \
  .global sym; \
  .hidden sym; \
  .type sym, %function; \
  .align 2; \
  sym

#define END_FUNC(sym) .size sym, .-sym

  .macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    tst sp, #7
    bne align_check_failed
#endif
  .endm

FUNC(fiber_asm_switch):
  stm r0!, {r3-r14}
  ldm r1!, {r3-r14}
  vstm r0, {d8-d15}
  vldm r1, {d8-d15}
  check_stack_alignment
  bx lr
END_FUNC(fiber_asm_switch)

FUNC(fiber_asm_invoke):
  pop {r0, r1}
  check_stack_alignment
  blx r1
  ldm sp, {r0, lr}
  mov sp, r0
  check_stack_alignment
  bx lr
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack):
  stmdb r2!, {v1, lr}
  mov v1, sp
  mov sp, r2
  check_stack_alignment
  blx r1
  mov r0, sp
  mov sp, v1
  ldm r0, {v1, pc}
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  b fiber_align_check_failed
#endif
