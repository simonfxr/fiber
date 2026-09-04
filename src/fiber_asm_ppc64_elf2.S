  .altmacro
  .machine power8
  .abiversion 2
  .text

#define CAT0(a, b) a ## b
#define CAT(a, b) CAT0(a, b)

#define FUNC_RAW(sym) \
  .hidden sym; \
  .globl sym; \
  .p2align 4; \
  .type sym,@function; \
  sym:

#define FUNC(sym) \
  FUNC_RAW(sym); \
  CAT(.begin_, sym): ; \
  addis 2,12,.TOC.-CAT(CAT(.begin_, sym), @ha); \
  addi 2,2,.TOC.-CAT(CAT(.begin_, sym), @l); \
  .localentry sym,.-sym

#define END_FUNC(sym) \
  .long 0; \
  .quad   0; \
  .size sym, .-sym

 .macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
   test esp, 0xF
   jnz .align_check_failed
#endif
 .endm

FUNC(fiber_asm_switch)

  mfcr 5
  stw 5, 0(3)
  lwz 5, 0(4)
  mtcr 5

  mfvrsave 6
  stw 6, 4(3)
  lwz 6, 4(4)
  mtvrsave 6

  mflr 0
  std 0, 8(3)
  ld 0, 8(4)
  mtlr 0

  std 1, 16(3)
  ld 1, 16(4)

  .set i, 0
  .rep 18
    std 14+i, 24+8*i(3)
    ld  14+i, 24+8*i(4)
    .set i, i+1
  .endr

  .set i, 0
  .rep 18
    stfd 14+i, 168+8*i(3)
    lfd  14+i, 168+8*i(4)
    .set i, i+1
  .endr

  // 16byte align beginning of vN in r3/r4
  addi 3, 3, (36 + 3) * 8 + 15
  rldicr 3, 3, 0, 59
  addi 4, 4, (36 + 3) * 8 + 15
  rldicr 4, 4, 0, 59

  stvx 20, 0, 3
  lvx 20, 0, 4
  .set i, 1
  .rep 11
    li 5, i * 16
    stvx 20+i, 3, 5
    lvx 20+i, 4, 5
    .set i, i+1
  .endr

  blr

END_FUNC(fiber_asm_switch)

FUNC_RAW(fiber_asm_invoke)
  ld 3, 0(1)
  ld 12, 8(1)
  mtctr 12
  stdu 1, -48(1)
  std 2, 24(1)
  bctrl
  ld 2, 24(1)
  ld 0, 72(1)
  mtlr 0
  ld 3, 64(1)
  mr 1, 3
  blr
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack)
  mflr 0
  mr 6, 31
  mr 12, 4
  mtctr 4
  mr 31, 1
  std 0, 16(1)
  mr 1, 5
  std 6, -8(1)
  stdu 1, -48(1)
  std 2, 24(1)
  bctrl
  ld 2, 24(1)
  mr 3, 31
  ld 31, 40(1)
  mr 1, 3
  ld 0,16(1)
  mtlr 0
  blr
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
.align_check_failed:
  b fiber_align_check_failed
#endif
