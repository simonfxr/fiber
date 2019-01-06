#ifndef FIBER_H
#define FIBER_H

#include <hu/macros.h>
#include <hu/platform.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef FIBER_SHARED
#ifdef fiber_EXPORTS
#define FIBER_API HU_LIB_EXPORT
#else
#define FIBER_API HU_LIB_IMPORT
#endif
#else
#define FIBER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t FiberState;

#ifdef __cplusplus
#define FIBER_CAST(T, x) static_cast<T>(x)
#define FIBER_STATE_CONSTANT(x) static_cast<::FiberState>(x)
#else
#define FIBER_CAST(T, x) ((T) x)
#define FIBER_STATE_CONSTANT(x) ((FiberState) x)
#endif

#define FIBER_FS_EXECUTING FIBER_STATE_CONSTANT(1)
#define FIBER_FS_TOPLEVEL FIBER_STATE_CONSTANT(2)
#define FIBER_FS_ALIVE FIBER_STATE_CONSTANT(4)
#define FIBER_FS_HAS_GUARD_PAGES FIBER_STATE_CONSTANT(8)

#if !HU_OS_POSIX_P || !HU_MACH_X86_P
#error "fiber: platform not supported"
#endif

#if defined(HU_BITS_32)
typedef struct
{
    void *sp;
    void *ebp;
    void *ebx;
    void *edi;
    void *esi;
} Regs;
#elif defined(HU_BITS_64)
typedef struct
{
    void *sp;
    void *rbp;
    void *rbx;
    void *r12;
    void *r13;
    void *r14;
    void *r15;
} Regs;
#else
#error "BITS not defined"
#endif

typedef struct
{
    Regs regs;
    void *stack;
    void *alloc_stack;
    size_t stack_size;
    FiberState state;
} Fiber;

typedef void (*FiberFunc)(void *);
typedef void (*FiberCleanupFunc)(Fiber *, void *);

FIBER_API Fiber *
fiber_init(Fiber *fiber,
           void *stack,
           size_t stack_size,
           FiberCleanupFunc cleanup,
           void *arg);

FIBER_API void
fiber_init_toplevel(Fiber *fiber);

FIBER_API bool
fiber_alloc(Fiber *fiber,
            size_t stack_size,
            FiberCleanupFunc cleanup,
            void *arg,
            bool use_guard_pages);

FIBER_API void
fiber_destroy(Fiber *fiber);

FIBER_API void
fiber_switch(Fiber *from, Fiber *to);

FIBER_API void
fiber_push_return(Fiber *fiber,
                  FiberFunc f,
                  const void *args,
                  size_t args_size);

FIBER_API void
fiber_reserve_return(Fiber *fiber,
                     FiberFunc f,
                     void **args_dest,
                     size_t args_size);

FIBER_API void
fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc f, void *args);

static inline bool
fiber_is_toplevel(Fiber *fiber)
{
    return (fiber->state & FIBER_FS_TOPLEVEL) != 0;
}

static inline bool
fiber_is_executing(Fiber *fiber)
{
    return (fiber->state & FIBER_FS_EXECUTING) != 0;
}

static inline bool
fiber_is_alive(Fiber *fiber)
{
    return (fiber->state & FIBER_FS_ALIVE) != 0;
}

static inline void
fiber_set_alive(Fiber *fiber, bool alive)
{
    if (alive)
        fiber->state |= FIBER_FS_ALIVE;
    else
        fiber->state &= ~FIBER_FS_ALIVE;
}

static inline void *
fiber_stack(const Fiber *fiber)
{
    return fiber->stack;
}

static inline size_t
fiber_stack_size(const Fiber *fiber)
{
    return fiber->stack_size;
}

static inline size_t
fiber_stack_free_size(const Fiber *fiber)
{
    return fiber->stack_size - (FIBER_CAST(char *, fiber->regs.sp) -
                                FIBER_CAST(char *, fiber->stack));
}

#ifdef __cplusplus
}
#endif

#endif
