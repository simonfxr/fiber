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
  pop rax

  .set i, 0
  .irp r, rsp, rax, rbx, rbp, rdi, rsi, r12, r13, r14, r15
     mov [rcx + 8 * i], \r
     mov \r, [rdx + 8 * i]
     .set i, i+1
  .endr

  lea rcx, [rcx + 11 * 8]
  and rcx, -16
  lea rdx, [rdx + 11 * 8]
  and rdx, -16

  .set i, 0
  .irp r, xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15
    movaps [rcx + 16 * i], \r
    movaps \r, [rdx + 16 * i]
    .set i, i+1
  .endr

  jmp rax

FUNC(fiber_asm_invoke):
  mov rcx, [rsp]
  mov rdx, [rsp+8]
  sub rsp, 32
  check_stack_alignment
  call rdx
  add rsp, 48
  mov rax, [rsp + 8]
  mov rsp, [rsp]
  jmp rax

FUNC(fiber_asm_exec_on_stack):
  push rbp
  mov rbp, rsp
  lea rsp, [r8 - 32]
  check_stack_alignment
  call rdx
  mov rsp, rbp
  pop rbp
  ret

#ifdef FIBER_ASM_CHECK_ALIGNMENT
align_check_failed:
  jmp fiber_align_check_failed
#endif
