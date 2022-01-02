#include <fiber/fiber_mach.h>

  .altmacro

#ifdef __pic__
  .option pic
#else
  .option nopic
#endif

  .text

#if HU_BITS_32_P
#  define W 4
#  define lx lw
#  define sx sw
#else
#  define W 8
#  define lx ld
#  define sx sd
#endif

#define FUNC(sym) \
  .global sym; \
  .hidden sym; \
  .type sym, @function; \
  .align 4; \
  sym
#define END_FUNC(sym) .size sym, .-sym

  .macro check_stack_alignment_nomove reg
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    andi \reg, \reg, FIBER_STACK_ALIGNMENT - 1
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

  sx sp, 0(a0)
  lx sp, 0(a1)
  sx ra, W(a0)
  lx ra, W(a1)

  .macro restore_s n
     sx s\n, 2*W+W*\n(a0)
     lx s\n, 2*W+W*\n(a1)
  .endm

  .set i, 0
  .rept 12
     restore_s %i
     .set i,i+1
  .endr

  .macro restore_fs n
     fsd fs\n, 14*W+8*\n(a0)
     fld fs\n, 14*W+8*\n(a1)
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
  lx a0, 0(sp)
  lx a1, W(sp)
  check_stack_alignment_move t1
  jalr a1
  lx ra, 3*W(sp)
  lx sp, 2*W(sp)
  check_stack_alignment_move t1
  ret
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack):
  addi sp, sp, -16
  sx ra, 0(sp)
  sx s0, W(sp)
  mv s0, sp
  mv sp, a2
  check_stack_alignment_move t1
  jalr a1
  mv sp, s0
  lx ra, 0(s0)
  lx s0, W(s0)
  addi sp, sp, 16
  ret
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  j fiber_align_check_failed
#endif
