  .altmacro

#ifdef __pic__
  .option pic
#else
  .option nopic
#endif

  .text

#define FUNC(sym) .align 4; .global sym; .hidden sym; .type sym, @function; sym
#define END_FUNC(sym) .size sym, .-sym

  .macro check_stack_alignment_nomove reg
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    andi \reg, \reg, 0xF
    bnez \reg, align_check_failed
#endif
  .endm

  .macro check_stack_alignment_move reg
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    mv \reg, sp
    check_stack_alignment_nomove \reg
#endif
  .endm

FUNC(fiber_asm_switch):

  sd sp, 0(a0)
  ld sp, 0(a1)
  sd ra, 8(a0)
  ld ra, 8(a1)

  .macro restore_s n
     sd s\n, 16+8*\n(a0)
     ld s\n, 16+8*\n(a1)
  .endm

  .set i, 0
  .rept 12
     restore_s %i
     .set i,i+1
  .endr

  .macro restore_fs n
     fsd fs\n, 16+12*8+8*\n(a0)
     fld fs\n, 16+12*8+8*\n(a1)
  .endm

  .set i, 0
  .rept 12
     restore_fs %i
     .set i,i+1
  .endr

  check_stack_alignment_move t1

  ret
END_FUNC(fiber_asm_switch)

FUNC(fiber_asm_invoke):
  ld a1, 0(sp)
  ld a0, 8(sp)
  check_stack_alignment_move t1
  jalr a1
  ld ra, 16(sp)
  ld sp, 24(sp)
  check_stack_alignment_move t1
  ret
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack):
  addi sp, sp, -16
  sd s0, 0(sp)
  sd ra, 8(sp)
  mv s0, sp
  mv sp, a0
  mv a0, a2
  check_stack_alignment_move t1
  jalr a1
  mv sp, s0
  ld ra, 8(s0)
  ld s0, 0(s0)
  addi sp, sp, 16
  ret
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  j fiber_align_check_failed
#endif
