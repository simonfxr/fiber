  .altmacro
  .intel_syntax noprefix
  .text

#include <hu/objfmt.h>

#define CONC0(a, b) a ## b
#define CONC(a, b) CONC0(a, b)

#if HU_OBJFMT_ELF_P
#define FUNC(sym) \
  .global sym; \
  .hidden sym; \
  .type sym, @function; \
  sym
#define END_FUNC(sym) .size sym, .-sym
#define ENTRY(sym) sym
#elif HU_OBJFMT_MACHO_P
#define FUNC(sym) \
  .global CONC(_, sym); \
  CONC(_, sym)
#define END_FUNC(sym)
#define ENTRY(sym) CONC(_, sym)
#else
#error "HU_OBJFMT_*_P neither ELF nor MACHO"
#endif

 .macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
   test esp, 0xF
   jnz .align_check_failed
#endif
 .endm


FUNC(fiber_asm_switch):
  pop rax

  check_stack_alignment

  mov rcx, [rdi]
  mov rdx, rsp
  sub rdx, rcx
  mov [rdi + 8 * 1], rdx

  mov rcx, [rsi]
  mov rdx, [rsi + 8 * 1]
  lea rsp, [rdx + rcx]

  .set i, 2
  .irp r, rax, rbp, rbx, r12, r13, r14, r15
     mov [rdi + 8 * i], \r
     mov \r, [rsi + 8 * i]
     .set i, i+1
  .endr

  check_stack_alignment

  jmp rax
END_FUNC(fiber_asm_switch)


FUNC(fiber_asm_invoke):
  check_stack_alignment
  mov rdi, [rsp]
  add rdi, rsp
  mov rsi, [rsp+8]
  call rsi
  mov rax, [rsp+16]
  add rsp, [rsp+24]
  check_stack_alignment
  jmp rax
END_FUNC(fiber_asm_invoke)


FUNC(fiber_asm_exec_on_stack):
  push rbp
  mov rbp, rsp
  mov rsp, rdx
  check_stack_alignment
  call rsi
  mov rsp, rbp
  pop rbp
  ret
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
.align_check_failed:
  jmp ENTRY(fiber_align_check_failed)
#endif
