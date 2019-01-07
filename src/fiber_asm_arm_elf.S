  .arch armv6
  .syntax unified
  .arm
  .fpu vfp
  .text

  .global fiber_asm_switch
  .hidden fiber_asm_switch
  .type   fiber_asm_switch, %function

  .global fiber_asm_invoke
  .hidden fiber_asm_invoke
  .type   fiber_asm_invoke, %function

  .global fiber_asm_exec_on_stack
  .hidden fiber_asm_exec_on_stack
  .type   fiber_asm_exec_on_stack, %function

  .align 2
fiber_asm_switch:
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  tst r13, #7
  beq 1f
  bl fiber_align_check_failed
1:
#endif
  push {lr}
  stm r0!, {r4-r13}
  vstm r0, {d9-d15}
  ldm r1!, {r4-r13}
  vldm r1, {d9-d15}
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  pop {lr}
  tst r13, #7
  beq 2f
  bl fiber_align_check_failed
2:
  bx lr
#else
  pop {pc}
#endif
  .size fiber_asm_switch, .-fiber_asm_switch

  .align 2
fiber_asm_invoke:
  pop {r0-r1}
  ldr r0, [sp]
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  tst sp, #7
  beq 1f
  bl fiber_align_check_failed
1:
#endif
  blx r1
  ldr r13, [r13, #4]
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  pop {lr}
  tst r13, #7
  beq 2f
  bl fiber_align_check_failed
2:
  bx lr
#else
  pop {pc}
#endif
  .size fiber_asm_invoke, .-fiber_asm_invoke

  .align 2
fiber_asm_exec_on_stack:
  stmdb r0!, {r3, r13, lr}
  mov r13, r0
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  tst r13, #7
  beq 1f
  bl fiber_align_check_failed
1:
#endif
  mov r0, r2
  blx r1
  pop {r0-r2}
  mov r13, r1
  bx r2
  .size fiber_asm_exec_on_stack, .-fiber_asm_exec_on_stack
