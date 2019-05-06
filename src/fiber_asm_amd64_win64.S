  .altmacro
  .intel_syntax noprefix
  .text

#define FUNC(sym) \
  .global sym; \
  .def    sym; \
  .scl    3; \
  .type   32; \
  .endef; \
  sym

.macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
   test esp, 0xF
   jnz align_check_failed
#endif
 .endm

FUNC(fiber_asm_switch):
  mov [rcx], rsp
  mov rax, [rdx]
  mov r8, [rax]
  lea rsp, [rax + 8]

  .set i, 1
  .irp r, rbx, rbp, rdi, rsi, r12, r13, r14, r15
     mov [rdi + 8 * i], \r
     mov \r, [rsi + 8 * i]
     .set i, i+1
  .endr

  lea rcx, [rcx + 10 * 8]
  and rcx, -16
  lea rdx, [rdx + 10 * 8]
  and rdx, -16

  .set i, 0
  .irp r, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15
    movaps [rcx + 16 * i], \r
    movaps \r, [rdx + 16 * i]
    .set i, i+1
  .endr

  jmp r8

FUNC(fiber_asm_invoke):
  mov rcx, [rsp+8]
  mov rdx, [rsp+16]
  sub rsp, 16
  check_stack_alignment
  call rdx
  mov rsp, [rsp + 40]
  ret

FUNC(fiber_asm_exec_on_stack):
  push rbp
  mov rbp, rsp
  lea rsp, [r8 - 40]
  check_stack_alignment
  call rdx
  mov rsp, rbp
  mov rbp, [rbp]
  ret

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  jmp ENTRY(fiber_align_check_failed)
#endif
