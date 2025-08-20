.machine ppc7400
.text
.align 2

.macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  rlwinm r0, r1, 0, 28, 31 ; r1 & 0xF
  cmpwi r0, 0
  bne _align_check_failed
#endif
.endm

; Switch fiber context: r3 = old fiber, r4 = new fiber
  .globl _fiber_asm_switch
_fiber_asm_switch:
  ; Save volatile state in old fiber
  mfcr r5
  stw  r5, 0(r3)
  mflr r0
  stw  r0, 4(r3)
  stw  r1, 8(r3)

  stw r13, 12(r3)
  stw r14, 16(r3)
  stw r15, 20(r3)
  stw r16, 24(r3)
  stw r17, 28(r3)
  stw r18, 32(r3)
  stw r19, 36(r3)
  stw r20, 40(r3)
  stw r21, 44(r3)
  stw r22, 48(r3)
  stw r23, 52(r3)
  stw r24, 56(r3)
  stw r25, 60(r3)
  stw r26, 64(r3)
  stw r27, 68(r3)
  stw r28, 72(r3)
  stw r29, 76(r3)
  stw r30, 80(r3)
  stw r31, 84(r3)

  stfd f14, 88(r3)
  stfd f15, 96(r3)
  stfd f16, 104(r3)
  stfd f17, 112(r3)
  stfd f18, 120(r3)
  stfd f19, 128(r3)
  stfd f20, 136(r3)
  stfd f21, 144(r3)
  stfd f22, 152(r3)
  stfd f23, 160(r3)
  stfd f24, 168(r3)
  stfd f25, 176(r3)
  stfd f26, 184(r3)
  stfd f27, 192(r3)
  stfd f28, 200(r3)
  stfd f29, 208(r3)
  stfd f30, 216(r3)
  stfd f31, 224(r3)

  ; Restore volatile state from new fiber
  lwz  r5, 0(r4)
  mtcrf 0xFF, r5
  lwz  r0, 4(r4)
  mtlr r0
  lwz  r1, 8(r4)

  lwz r13, 12(r4)
  lwz r14, 16(r4)
  lwz r15, 20(r4)
  lwz r16, 24(r4)
  lwz r17, 28(r4)
  lwz r18, 32(r4)
  lwz r19, 36(r4)
  lwz r20, 40(r4)
  lwz r21, 44(r4)
  lwz r22, 48(r4)
  lwz r23, 52(r4)
  lwz r24, 56(r4)
  lwz r25, 60(r4)
  lwz r26, 64(r4)
  lwz r27, 68(r4)
  lwz r28, 72(r4)
  lwz r29, 76(r4)
  lwz r30, 80(r4)
  lwz r31, 84(r4)

  lfd  f14, 88(r4)
  lfd  f15, 96(r4)
  lfd  f16, 104(r4)
  lfd  f17, 112(r4)
  lfd  f18, 120(r4)
  lfd  f19, 128(r4)
  lfd  f20, 136(r4)
  lfd  f21, 144(r4)
  lfd  f22, 152(r4)
  lfd  f23, 160(r4)
  lfd  f24, 168(r4)
  lfd  f25, 176(r4)
  lfd  f26, 184(r4)
  lfd  f27, 192(r4)
  lfd  f28, 200(r4)
  lfd  f29, 208(r4)
  lfd  f30, 216(r4)
  lfd  f31, 224(r4)

  blr

; Invoke: r1 points to stack
  .globl _fiber_asm_invoke
_fiber_asm_invoke:
  lwz r3, 0(r1)    ; arg
  lwz r12, 4(r1)   ; func
  mtlr r12
  stwu r1, -48(r1) ; new frame
  stw  r2, 8(r1)
  blrl
  lwz r2, 8(r1)
  lwz r0, 44(r1)
  mtlr r0
  lwz r3, 40(r1)
  mr  r1, r3
  blr

; Execute on stack: r4=function, r5=stack, r6=arg, r31 preserved
  .globl _fiber_asm_exec_on_stack
_fiber_asm_exec_on_stack:
  mflr r0
  mr   r7, r31
  mr   r12, r4
  mtlr r12
  mr   r31, r1
  stw  r0, 8(r1)
  mr   r1, r5
  stw  r7, -4(r1)
  stwu r1, -48(r1)
  stw  r2, 8(r1)
  mr   r3, r6
  blrl
  lwz  r2, 8(r1)
  mr   r3, r31
  lwz  r31, 44(r1)
  mr   r1, r3
  lwz  r0, 8(r1)
  mtlr r0
  blr

#ifdef FIBER_ASM_CHECK_ALIGNMENT
_align_check_failed:
  b _fiber_align_check_failed
#endif
