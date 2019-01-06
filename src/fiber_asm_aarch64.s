.arch armv8-a
.text

.align 2
.global fiber_asm_switch
.type fiber_asm_switch, %function
fiber_asm_switch:
  stp xzr, x30, [sp, -16]!
  mov x3, sp
  stp x3, x19, [x0], 16
  stp x20, x21, [x0], 16
  stp x22, x23, [x0], 16
  stp x24, x25, [x0], 16
  stp x26, x27, [x0], 16
  stp x28, x29, [x0], 16
  stp d8, d9, [x0], 16
  stp d10, d11, [x0], 16
  stp d12, d13, [x0], 16
  stp d14, d15, [x0]

  ldp x2, x19, [x1], 16
  ldp x20, x21, [x1], 16
  ldp x22, x23, [x1], 16
  ldp x24, x25, [x1], 16
  ldp x26, x27, [x1], 16
  ldp x28, x29, [x1], 16
  ldp d8, d9, [x1], 16
  ldp d10, d11, [x1], 16
  ldp d12, d13, [x1], 16
  ldp d14, d15, [x1]

  ldp x0, x30, [x2], 16
  mov sp, x2
  tst x2, 0xF
  beq 1f
  b .
1:
  ret
.size fiber_asm_switch, .-fiber_asm_switch

.align 2
.global fiber_asm_invoke
.type fiber_asm_invoke, %function
fiber_asm_invoke:
  ldp x2, x1, [sp], 16
  mov x2, sp
  tst x2, 0xF
  beq 1f
  b .
1:
  ldr x0, [sp]
  blr x1
  ldp x0, x1, [sp], 16
  mov sp, x1
  tst x1, 0xF
  beq 1f
  b .
1:
  ldp x0, x30, [sp], 16
  ret
.size fiber_asm_invoke, .-fiber_asm_invoke


.align 2
.global fiber_asm_exec_on_stack
.type fiber_asm_exec_on_stack, %function
fiber_asm_exec_on_stack:
  mov x3, sp
  stp x3, x30, [x0, -16]!
  mov sp, x0
  tst x0, 0xF
  beq 1f
  b .
1:
  mov x0, x2
  blr x1
  ldp x3, x30, [sp]
  mov sp, x3
  ret
.size fiber_asm_exec_on_stack, .-fiber_asm_exec_on_stack
