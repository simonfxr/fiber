  .altmacro
  .intel_syntax noprefix
  .text


#ifdef FIBER_STACK_ALIGNMENT
#define STACK_ALIGNMENT FIBER_STACK_ALIGNMENT
#else
#define STACK_ALIGNMENT 16
#endif

#define FUNC(sym) \
  .global sym; \
  .hidden sym; \
  .type sym, @function; \
  sym
#define END_FUNC(sym) .size sym, .-sym

  .macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    test esp, STACK_ALIGNMENT - 1
    jnz .align_check_failed
#endif
  .endm

FUNC(fiber_asm_switch):
  mov edx, [esp+4]
  lea ecx, [esp+4]
  sub ecx, [edx]
  mov [edx+4*1], ecx

  mov ecx, [esp+8]
  mov eax, [ecx]
  add eax, [ecx+4*1]
  xchg esp, eax
  mov eax, [eax]

  check_stack_alignment

  .set i, 2
  .irp r, eax, ebp, ebx, edi, esi
    mov [edx+4*i], \r
    mov \r, [ecx+4*i]
    .set i, i+1
  .endr
  jmp eax
END_FUNC(fiber_asm_switch)

FUNC(fiber_asm_invoke):
  check_stack_alignment
  add [esp], esp
  call [esp+4]
  mov eax, [esp+8]
  add esp, [esp+12]
  check_stack_alignment
  jmp eax
END_FUNC(fiber_asm_invoke)

FUNC(fiber_asm_exec_on_stack):
  mov eax, esp
  mov edx, [eax+4]
  mov ecx, [eax+8]
  mov esp, [eax+12]
  sub esp, 16
  mov [esp+4], eax
  mov [esp], edx
  check_stack_alignment
  call ecx
  mov esp, [esp+4]
  ret
END_FUNC(fiber_asm_exec_on_stack)

#ifdef FIBER_ASM_CHECK_ALIGNMENT
.align_check_failed:
  jmp fiber_align_check_failed
#endif
