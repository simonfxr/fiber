#ifndef FIBER_MACH_H
#define FIBER_MACH_H

#include <hu/arch.h>
#include <hu/bits.h>
#include <hu/os.h>

#ifndef FIBER_H
#error "include fiber.h before fiber_mach.h"
#endif

#if HU_BITS_32_P && HU_OS_POSIX_P && HU_ARCH_X86_P
#define FIBER_TARGET_32_CDECL 1
typedef struct
{
    void *sp;
    void *ebp;
    void *ebx;
    void *edi;
    void *esi;
} FiberRegs;
#elif HU_BITS_64_P && HU_OS_POSIX_P && HU_ARCH_X86_P
#define FIBER_TARGET_64_SYSV 1
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
#define FIBER_TARGET_32_ARM_EABI
typedef struct
{
    void *r4;
    void *r5;
    void *r6;
    void *r7;
    void *r8;
    void *r9;
    void *r10;
    void *r11;
    void *r12;
    void *sp;
    uint64_t d8;
    uint64_t d9;
    uint64_t d10;
    uint64_t d11;
    uint64_t d12;
    uint64_t d13;
    uint64_t d14;
    uint64_t d15;
} FiberRegs;
#elif HU_BITS_64_P && HU_OS_POSIX_P && HU_ARCH_ARM_P
#define FIBER_TARGET_64_AARCH 1
typedef struct
{
    void *sp;
    void *r19;
    void *r20;
    void *r21;
    void *r22;
    void *r23;
    void *r24;
    void *r25;
    void *r26;
    void *r27;
    void *r28;
    void *r29;
    // only low 64 bits have to be preserved
    uint64_t v8;
    uint64_t v9;
    uint64_t v10;
    uint64_t v11;
    uint64_t v12;
    uint64_t v13;
    uint64_t v14;
    uint64_t v15;
} FiberRegs;
#elif HU_OS_WINDOWS_P && HU_BITS_64_P && HU_ARCH_X86_P
#define FIBER_TARGET_64_WIN 1
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
    char xmm[168];
} FiberRegs;
#elif HU_OS_WINDOWS_P && HU_BITS_32_P && HU_ARCH_X86_P
#define FIBER_TARGET_32_WIN 1
#define FIBER_CCONV __cdecl
typedef struct
{
    void *sp;
    void *ebx;
    void *ebp;
    void *esi;
    void *edi;
} FiberRegs;
#else
#error "fiber: system/architecture target not supported"
#endif

#ifndef FIBER_CCONV
#define FIBER_CCONV
#endif

#endif
