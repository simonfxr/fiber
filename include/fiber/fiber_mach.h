#ifndef FIBER_MACH_H
#define FIBER_MACH_H

#include <hu/arch.h>
#include <hu/bits.h>
#include <hu/os.h>

#ifndef FIBER_H
#error "include fiber.h before fiber_mach.h"
#endif

#if HU_BITS_32_P && HU_OS_POSIX_P && HU_ARCH_X86_P
#define FIBER_TARGET_X86_CDECL 1
#define FIBER_DEFAULT_STACK_ALIGNMENT 16
typedef struct
{
    void *sp;
    void *ebp;
    void *ebx;
    void *edi;
    void *esi;
} FiberRegs;
#elif HU_BITS_64_P && HU_OS_POSIX_P && HU_ARCH_X86_P
#define FIBER_TARGET_AMD64_SYSV
#define FIBER_STACK_ALIGNMENT 16
typedef struct
{
    void *sp;
    void *rbp;
    void *rbx;
    void *r12;
    void *r13;
    void *r14;
    void *r15;
} FiberRegs;
#elif HU_BITS_32_P && HU_OS_POSIX_P && HU_ARCH_ARM_P
#define FIBER_TARGET_ARM32_EABI
#define FIBER_DEFAULT_STACK_ALIGNMENT 8
#define FIBER_HAVE_LR 1
typedef struct
{
    void *__pad;
    void *r[9];  /* r4 - r12 */
    void *sp;    /* r13 */
    void *lr;    /* r14 */
    double d[8]; /* d8 - d15 */
} FiberRegs;
#elif HU_BITS_64_P && HU_OS_POSIX_P && HU_ARCH_ARM_P
#define FIBER_TARGET_AARCH64_APCS 1
#define FIBER_DEFAULT_STACK_ALIGNMENT 16
#define FIBER_HAVE_LR 1
typedef struct
{
    void *sp;
    void *lr;    /* r30 */
    void *fp;    /* r29 */
    void *r[10]; /* r19 - r28 */
    double d[8]; /* d8 - d15 */
} FiberRegs;
#elif HU_OS_WINDOWS_P && HU_BITS_64_P && HU_ARCH_X86_P
#define FIBER_TARGET_AMD64_WIN64 1
#define FIBER_DEFAULT_STACK_ALIGNMENT 16
typedef struct
{
    void *sp;
    void *rbx;
    void *rbp;
    void *rdi;
    void *rsi;
    void *r12;
    void *r13;
    void *r14;
    void *r15;
    /* 10 * 16 bytes, add aditional 8 bytes to make 16byte alignment possible */
    double xmm[21];
} FiberRegs;
#elif HU_OS_WINDOWS_P && HU_BITS_32_P && HU_ARCH_X86_P
#define FIBER_TARGET_X86_WIN32 1
#define FIBER_CCONV __cdecl
#define FIBER_DEFAULT_STACK_ALIGNMENT 4
typedef struct
{
    void *sp;
    void *ebx;
    void *ebp;
    void *esi;
    void *edi;
} FiberRegs;
#elif HU_OS_LINUX_P && HU_BITS_64_P && HU_ARCH_RISCV_P
#define FIBER_TARGET_RISCV64_ELF 1
#define FIBER_DEFAULT_STACK_ALIGNMENT 16
#define FIBER_HAVE_LR 1
#ifndef __riscv_float_abi_double
#error "this RISCV abi is not supported use -mabi=lp64d"
#endif
typedef struct
{
    void *sp;
    void *lr;
    void *s[12];
    double fs[12];
} FiberRegs;
#else
#error "fiber: system/architecture target not supported"
#endif

#ifndef FIBER_CCONV
#define FIBER_CCONV
#endif

#endif
