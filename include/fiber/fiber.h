#ifndef FIBER_H
#define FIBER_H

#include <hu/annotations.h>
#include <hu/lang.h>
#include <hu/objfmt.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef FIBER_SHARED
#ifdef fiber_EXPORTS
#define FIBER_API HU_DSO_EXPORT
#else
#define FIBER_API HU_DSO_IMPORT
#endif
#else
#define FIBER_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <fiber/fiber_mach.h>

typedef uint16_t FiberState;
typedef uint16_t FiberFlags;

/**
 * A Fiber represents a couroutine which has its own call stack and a set of
 * preserved registers.
 */
typedef struct Fiber
{
    FiberRegs regs;
    void *stack;
    void *alloc_stack;
    size_t stack_size;
    FiberState state;
} Fiber;

#define FIBER_STATE_CONSTANT(x) HU_STATIC_CAST(FiberState, x)
#define FIBER_FLAG_CONSTANT(x) HU_STATIC_CAST(FiberFlags, x)

#define FIBER_FS_EXECUTING FIBER_STATE_CONSTANT(1)
#define FIBER_FS_TOPLEVEL FIBER_STATE_CONSTANT(2)
#define FIBER_FS_ALIVE FIBER_STATE_CONSTANT(4)
#define FIBER_FS_HAS_LO_GUARD_PAGE FIBER_STATE_CONSTANT(8)
#define FIBER_FS_HAS_HI_GUARD_PAGE FIBER_STATE_CONSTANT(16)

#define FIBER_FLAG_GUARD_LO FIBER_FLAG_CONSTANT(8)
#define FIBER_FLAG_GUARD_HI FIBER_FLAG_CONSTANT(16)

typedef void(FIBER_CCONV *FiberFunc)(void *);
typedef void(FIBER_CCONV *FiberCleanupFunc)(Fiber *, void *);

/**
 * initialize a Fiber with a preallocated stack. Stack alignment will be
 * correctly handled, this means in most cases the usuable stack size is
 * slightly less then the size of the stack buffer. The cleanup function
 * should never return! There is no stack frame where it could return to. In
 * practice this means that this function should switch to some other fiber
 * (usually the toplevel fiber).
 * @param fiber the fiber to create
 * @param stack preallocated stack
 * @param stack_size size of preallocated stack
 * @param cleanup the initial function on the call stack
 * @param arg the arg to pass to cleanup when it is invoked
 */
FIBER_API
HU_NONNULL_PARAMS(1, 2, 4)
HU_RETURNS_NONNULL
Fiber *
fiber_init(HU_OUT_NONNULL Fiber *fiber,
           HU_INOUT_NONNULL void *stack,
           size_t stack_size,
           HU_IN_NONNULL FiberCleanupFunc cleanup,
           void *arg);

/**
 * initialize a toplevel Fiber, a toplevel Fiber represents an OS thread. Should
 * only be called once per OS Thread!
 * @param fiber the fiber to create
 */
FIBER_API
HU_NONNULL_PARAMS(1)
void
fiber_init_toplevel(HU_OUT_NONNULL Fiber *fiber);

/**
 * create a new Fiber by allocating a fresh stack, optionally with bottom or top
 * guard frames (each usually adds an overhead of 4kb). It is recommended to
 * pass FIBER_FLAG_GUARD_LO, to catch stack overflows. @see fiber_init()
 * @param fiber the fiber to create
 * @param stack_size size of stack
 * @param cleanup the initial function on the call stack.
 * @param arg the arg to pass to cleanup when it is invoked
 */
FIBER_API
HU_NONNULL_PARAMS(1, 3)
HU_NODISCARD
bool
fiber_alloc(HU_OUT_NONNULL Fiber *fiber,
            size_t stack_size,
            HU_IN_NONNULL FiberCleanupFunc cleanup,
            void *arg,
            FiberFlags flags);

/**
 * Deallocate the stack, does nothing if created by fiber_init().
 * @param fiber the fiber to destroy
 */
FIBER_API
HU_NONNULL_PARAMS(1)
void
fiber_destroy(HU_IN_NONNULL Fiber *fiber);

/**
 * Switch from the current fiber to a different fiber by returning to the stack
 * frame of the new fiber. from has to be the active fiber!
 * @param from currently executing fiber
 * @param to fiber to switch to
 */
FIBER_API
HU_NONNULL_PARAMS(1, 2)
void
fiber_switch(HU_INOUT_NONNULL Fiber *from, HU_INOUT_NONNULL Fiber *to);

/**
 * Allocate a fresh stack frame at the top of a fiber with an argument buffer of
 * args_size. If the fiber is switched to it will execute the function.
 * @param fiber the fiber where the new frame will be allocated, cannot be the
 * currently executing fiber!
 * @param f the function to place in the stack frame, it will be called with a
 * pointer to the beginning of the argument buffer
 * @param args_dest the argument will receive a pointer to the buffer allocated
 * on the fiber stack, will be aligned to 8 bytes on all platforms.
 * @param args_size size of the argument buffer to allocate
 */
HU_NONNULL_PARAMS(1, 2, 3)
FIBER_API void
fiber_reserve_return(HU_INOUT_NONNULL Fiber *fiber,
                     HU_IN_NONNULL FiberFunc f,
                     HU_OUT_NONNULL void **args_dest,
                     size_t args_size);
/**
 * similar to @see fiber_reserve_return(), instead of returning the pointer to
 * the buffer, it will copy the contents into the stack allocated argument
 * buffer
 * @param fiber where to allocate the fresh stack frame
 * @param f function to place into the stack frame
 * @param args buffer to copy onto the stack frame
 * @param args_size size of argument buffer to copy
 */
HU_NONNULL_PARAMS(1, 2)
static inline void
fiber_push_return(HU_INOUT_NONNULL Fiber *fbr,
                  HU_IN_NONNULL FiberFunc f,
                  const void *args,
                  size_t s)
{
    void *args_dest;
    fiber_reserve_return(fbr, f, &args_dest, s);
    memcpy(args_dest, args, s);
}

/**
 * Temporarily switch from the active fiber to a temporary fiber and immediately
 * execute a function on this stack. After the call returns switch back to the
 * original fiber. This is useful if a call might consume a lot of stack space,
 * in this case temp should be a toplevel fiber.
 * @param active currently executing fiber
 * @param temp fiber where the call to f will take place
 * @param f function to call
 * @param argument to pass to f
 */
FIBER_API
HU_NONNULL_PARAMS(1, 2, 3)
void
fiber_exec_on(HU_IN_NONNULL Fiber *active,
              HU_INOUT_NONNULL Fiber *temp,
              HU_IN_NONNULL FiberFunc f,
              void *args);

HU_WARN_UNUSED
static inline bool
fiber_is_toplevel(const Fiber *fiber)
{
    return (fiber->state & FIBER_FS_TOPLEVEL) != 0;
}

HU_WARN_UNUSED
HU_NONNULL_PARAMS(1)
static inline bool
fiber_is_executing(HU_IN_NONNULL const Fiber *fiber)
{
    return (fiber->state & FIBER_FS_EXECUTING) != 0;
}

HU_WARN_UNUSED
HU_NONNULL_PARAMS(1)
static inline bool
fiber_is_alive(HU_IN_NONNULL const Fiber *fiber)
{
    return (fiber->state & FIBER_FS_ALIVE) != 0;
}

HU_NONNULL_PARAMS(1)
static inline void
fiber_set_alive(HU_INOUT_NONNULL Fiber *fiber, bool alive)
{
    if (alive)
        fiber->state |= FIBER_FS_ALIVE;
    else
        fiber->state &= ~FIBER_FS_ALIVE;
}

HU_WARN_UNUSED
HU_NONNULL_PARAMS(1)
static inline void *
fiber_stack(HU_IN_NONNULL const Fiber *fiber)
{
    return fiber->stack;
}

HU_WARN_UNUSED
HU_NONNULL_PARAMS(1)
static inline size_t
fiber_stack_size(HU_IN_NONNULL const Fiber *fiber)
{
    return fiber->stack_size;
}

HU_WARN_UNUSED
HU_NONNULL_PARAMS(1)
static inline size_t
fiber_stack_free_size(HU_IN_NONNULL const Fiber *fiber)
{
    return fiber->stack_size - (HU_STATIC_CAST(char *, fiber->regs.sp) -
                                HU_STATIC_CAST(char *, fiber->stack));
}

#ifdef __cplusplus
}
#endif
#endif
