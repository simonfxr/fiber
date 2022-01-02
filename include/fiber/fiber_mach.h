#ifndef FIBER_MACH_H
#define FIBER_MACH_H

#ifndef FIBER_AMALGAMATED
#    include <hu/arch.h>
#    include <hu/bits.h>
#    include <hu/endian.h>
#    include <hu/lang.h>
#    include <hu/objfmt.h>
#    include <hu/os.h>
#endif

#if HU_ARCH_X86_P && HU_BITS_32_P && (HU_OS_POSIX_P || HU_OS_WINDOWS_P)
#    if HU_OS_WINDOWS_P
#        define FIBER_TARGET_X86_WIN32 1
#        define FIBER_CCONV __cdecl
#        define FIBER_DEFAULT_STACK_ALIGNMENT 4
#    else
#        define FIBER_TARGET_X86_CDECL 1
#        if (HU_OS_FREEBSD_P || HU_OS_WINDOWS_P)
#            define FIBER_DEFAULT_STACK_ALIGNMENT 4
#        else
#            define FIBER_DEFAULT_STACK_ALIGNMENT 16
#        endif
#    endif
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;                                                              \
        void *ebp;                                                             \
        void *ebx;                                                             \
        void *edi;                                                             \
        void *esi

#elif HU_ARCH_X86_P && HU_BITS_64_P && HU_OS_POSIX_P
#    define FIBER_TARGET_AMD64_SYSV
#    define FIBER_STACK_ALIGNMENT 16
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;                                                              \
        void *rbp;                                                             \
        void *rbx;                                                             \
        void *r12;                                                             \
        void *r13;                                                             \
        void *r14;                                                             \
        void *r15

#elif HU_ARCH_X86_P && HU_BITS_64_P && HU_OS_WINDOWS_P
#    define FIBER_TARGET_AMD64_WIN64 1
#    define FIBER_DEFAULT_STACK_ALIGNMENT 16
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;                                                              \
        void *rbx;                                                             \
        void *rbp;                                                             \
        void *rdi;                                                             \
        void *rsi;                                                             \
        void *r12;                                                             \
        void *r13;                                                             \
        void *r14;                                                             \
        void *r15;                                                             \
        /* 10 * 16 bytes, add aditional 8 bytes to make 16byte alignment       \
         * possible */                                                         \
        double xmm[21]

#elif HU_ARCH_X86_P && HU_BITS_32_P && HU_OS_WINDOWS_P
#    define FIBER_TARGET_X86_WIN32 1
#    define FIBER_CCONV __cdecl
#    define FIBER_DEFAULT_STACK_ALIGNMENT 4
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;                                                              \
        void *ebp;                                                             \
        void *ebx;                                                             \
        void *edi;                                                             \
        void *esi

#elif HU_ARCH_ARM_P && HU_BITS_32_P && HU_OS_POSIX_P
#    define FIBER_TARGET_ARM32_EABI
#    define FIBER_DEFAULT_STACK_ALIGNMENT 8
#    define FIBER_ARCH_REGS                                                    \
        void *__pad;                                                           \
        void *r[9]; /* r4 - r12 */                                             \
        void *sp;   /* r13 */                                                  \
        void *lr;   /* r14 */                                                  \
        double d[8] /* d8 - d15 */

#elif HU_ARCH_ARM_P && HU_BITS_64_P && HU_OS_POSIX_P
#    define FIBER_TARGET_AARCH64_APCS 1
#    define FIBER_DEFAULT_STACK_ALIGNMENT 16
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;    /* r30 */                                                 \
        void *fp;    /* r29 */                                                 \
        void *r[10]; /* r19 - r28 */                                           \
        double d[8]; /* d8 - d15 */

#elif HU_ARCH_RISCV_P && HU_BITS_64_P && HU_OS_POSIX_P
#    define FIBER_TARGET_RISCV64_ELF 1
#    define FIBER_DEFAULT_STACK_ALIGNMENT 16
#    ifndef __riscv_float_abi_double
#        error "this RISCV abi is not supported use -mabi=lp64d"
#    endif
#    define FIBER_ARCH_REGS                                                    \
        void *sp;                                                              \
        void *lr;                                                              \
        void *s[12];                                                           \
        double fs[12]

#elif HU_ARCH_PPC_P && HU_BITS_64_P && HU_LITTLE_ENDIAN_P && HU_OBJFMT_ELF_P
#    define FIBER_TARGET_PPC64LE_ELF 1
#    define FIBER_DEFAULT_STACK_ALIGNMENT 16
#    if !defined(_CALL_ELF) || _CALL_ELF != 2
#        error "this PowerPC ABI is not supported, use -mabi=elfv2"
#    endif
#    define FIBER_ARCH_REGS                                                    \
        uint32_t cr;                                                           \
        uint32_t vrsave;                                                       \
        void *lr;             /* r0 */                                         \
        void *sp;             /* r1 */                                         \
        void *r[18];          /* r14 - r31 */                                  \
        double *f[18];        /* f14 - f31 */                                  \
        double *v[12 * 2 + 1] /* v20 - v31, adjusted to be 16 byte aligned */

#else
#    error "fiber: system/architecture target not supported"
#endif

#ifndef FIBER_CCONV
#    define FIBER_CCONV
#endif

#if HU_C_P || HU_CXX_P
HU_BEGIN_EXTERN_C
typedef struct FiberRegs
{
    FIBER_ARCH_REGS;
} FiberRegs;
HU_END_EXTERN_C
#endif

#endif
