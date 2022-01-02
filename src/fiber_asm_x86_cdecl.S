  .altmacro
  .intel_syntax noprefix
  .text

#include <fiber/fiber_mach.h>

#ifdef FIBER_STACK_ALIGNMENT
#define STACK_ALIGNMENT FIBER_STACK_ALIGNMENT
#else
#define STACK_ALIGNMENT FIBER_DEFAULT_STACK_ALIGNMENT
#endif

#define CONC0(a, b) a ## b
#define CONC(a, b) CONC0(a, b)

#if HU_OS_WINDOWS_P
#  define ENTRY(sym) CONC(_, sym)
#  define FUNC(sym) \
  .global ENTRY(sym); \
  .def    ENTRY(sym); \
  .scl    2; \
  .type   32; \
  .endef; \
  ENTRY(sym)
#  define END_FUNC(sym)

#else
#  define ENTRY(sym) sym
#  define FUNC(sym) \
  .global sym; \
  .hidden sym; \
  .type sym, @function; \
  sym

#  define END_FUNC(sym) .size sym, .-sym

#endif

  .macro check_stack_alignment
#ifdef FIBER_ASM_CHECK_ALIGNMENT
    test esp, STACK_ALIGNMENT - 1
    jnz .align_check_failed
#endif
  .endm

FUNC(fiber_asm_switch):
  pop eax
  mov edx, [esp]
  mov ecx, [esp+4]
  .set i, 0
  .irp r, esp, eax, ebp, ebx, edi, esi
    mov [edx+4*i], \r
    mov \r, [ecx+4*i]
    .set i, i+1
  .endr
  jmp eax
END_FUNC(fiber_asm_switch)

FUNC(fiber_asm_invoke):
  mov eax, [esp+4]
  check_stack_alignment
  call eax
  mov eax, [esp+12]
  mov esp, [esp+8]
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
  jmp ENTRY(fiber_align_check_failed)
#endif
