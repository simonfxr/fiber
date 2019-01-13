  .intel_syntax noprefix
  .text

#define CONC0(a, b) a ## b
#define CONC(a, b) CONC0(a, b)

#ifdef FIBER_ELF
#define GLOBAL(sym) .global sym; .hidden sym; .type sym, @function; sym
#define END_GLOBAL(sym) .size sym, .-sym
#define ENTRY(sym) sym
#elif defined(FIBER_MACHO)
#define GLOBAL(sym) .global CONC(_, sym); CONC(_, sym)
#define END_GLOBAL(sym)
#define ENTRY(sym) CONC(_, sym)
#endif


GLOBAL(fiber_asm_switch):           // Regs *from, Regs *to -> void
  mov rax, [rsi]
  mov rcx, [rax]

  mov [rdi + 0], rsp
  mov [rdi + 8], rbp
  mov [rdi + 16], rbx
  mov [rdi + 24], r12
  mov [rdi + 32], r13
  mov [rdi + 40], r14
  mov [rdi + 48], r15

  lea rsp, [rax + 8]
  mov rbp, [rsi + 8]
  mov rbx, [rsi + 16]
  mov r12, [rsi + 24]
  mov r13, [rsi + 32]
  mov r14, [rsi + 40]
  mov r15, [rsi + 48]

  jmp rcx
END_GLOBAL(fiber_asm_switch)


GLOBAL(fiber_asm_invoke):
  add rsp, 8              // skip filler for alignment
  pop rsi                 // function to call
  mov rdi, [rsp]          // rdi points to beginning of arguments

#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jnz ENTRY(fiber_align_check_failed)
#endif
  call rsi
  mov rsp, [rsp + 8]
  ret
END_GLOBAL(fiber_asm_invoke)


GLOBAL(fiber_asm_exec_on_stack):  // stack_ptr, void (*)(void *), void * -> void
  push rbp
  mov rbp, rsp
  lea rsp, [rdi - 8]
#ifdef FIBER_ASM_CHECK_ALIGNMENT
  test esp, 0xF
  jnz ENTRY(fiber_align_check_failed)
#endif
  mov rdi, rdx
  call rsi
  mov rax, rbp
  mov rsp, rbp
  pop rbp
  ret
END_GLOBAL(fiber_asm_exec_on_stack)
