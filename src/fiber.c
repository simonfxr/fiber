#include <fiber/fiber.h>

#include "fiber_asm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define STACK_ALIGNMENT ((uintptr_t) 16)

#ifdef FIBER_TARGET_32_CDECL
#define WORD_SIZE ((size_t) 4)
#define RED_ZONE ((size_t) 0)
#elif defined(FIBER_TARGET_64_SYSV)
#define WORD_SIZE ((size_t) 8)
#define RED_ZONE ((size_t) 128)
#elif defined(FIBER_TARGET_64_AARCH)
#define WORD_SIZE ((size_t) 8)
#define RED_ZONE ((size_t) 0)
#else
#error "FIBER_TARGET_* not defined"
#endif

#define UNUSED(a) ((void) (a))

#define IS_STACK_ALIGNED(x) (((uintptr_t)(x) & (STACK_ALIGNMENT - 1)) == 0)
#define STACK_ALIGN(x) ((uintptr_t)(x) & ~(STACK_ALIGNMENT - 1))

typedef struct
{
    Fiber *fiber;
    FiberCleanupFunc cleanup;
    void *arg;
} FiberGuardArgs;

static void
fiber_guard(void *);

static void
fiber_init_(Fiber *fbr, FiberCleanupFunc cleanup, void *arg)
{
    memset(&fbr->regs, 0, sizeof fbr->regs);
    uintptr_t sp =
      (uintptr_t)((char *) fbr->stack + fbr->stack_size - WORD_SIZE);
    sp &= ~(STACK_ALIGNMENT - 1);
    fbr->regs.sp = (void *) sp;
    FiberGuardArgs *args;
    fiber_reserve_return(fbr, fiber_guard, (void **) &args, sizeof *args);
    args->fiber = fbr;
    args->cleanup = cleanup;
    args->arg = arg;
    fbr->state |= FIBER_FS_ALIVE;
}

Fiber *
fiber_init(Fiber *fbr,
           void *stack,
           size_t stack_size,
           FiberCleanupFunc cleanup,
           void *arg)
{
    assert(fbr && "Fiber can not be null");
    fbr->stack = stack;
    fbr->stack_size = stack_size;
    fbr->alloc_stack = NULL;
    fbr->state = 0;
    fiber_init_(fbr, cleanup, arg);
    return fbr;
}

void
fiber_init_toplevel(Fiber *fbr)
{
    fbr->stack = NULL;
    fbr->stack_size = (size_t) -1;
    fbr->alloc_stack = NULL;
    memset(&fbr->regs, 0, sizeof fbr->regs);
    fbr->state = FIBER_FS_ALIVE | FIBER_FS_TOPLEVEL | FIBER_FS_EXECUTING;
}

static const size_t PAGE_SIZE = 4096;

bool
fiber_alloc(Fiber *fbr,
            size_t size,
            FiberCleanupFunc cleanup,
            void *arg,
            bool use_guard_pages)
{
    assert(fbr && "Fiber can not be null");
    fbr->stack_size = size;
    const size_t stack_size = size;

    if (!use_guard_pages) {
        fbr->alloc_stack = fbr->stack = malloc(stack_size);
        if (!fbr->alloc_stack)
            return false;
    } else {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE + 2;
        size_t byte_size = npages * PAGE_SIZE;
        if (posix_memalign(&fbr->alloc_stack, PAGE_SIZE, byte_size))
            return false;

        if (mprotect(fbr->alloc_stack, PAGE_SIZE, PROT_NONE) == -1)
            goto fail;

        if (mprotect(fbr->alloc_stack + (npages - 1) * PAGE_SIZE,
                     PAGE_SIZE,
                     PROT_NONE) == -1)
            goto fail;

        fbr->stack = fbr->alloc_stack + PAGE_SIZE;
    }

    fbr->state = FIBER_FS_HAS_GUARD_PAGES;
    fiber_init_(fbr, cleanup, arg);
    return true;

fail:
    free(fbr->alloc_stack);
    return false;
}

void
fiber_destroy(Fiber *fbr)
{
    assert(!fiber_is_executing(fbr));
    assert(!fiber_is_toplevel(fbr));

    if (!fbr->alloc_stack)
        return;

    if (fbr->state & FIBER_FS_HAS_GUARD_PAGES) {
        size_t npages = (fbr->stack_size + PAGE_SIZE - 1) / PAGE_SIZE + 2;
        mprotect(fbr->alloc_stack, PAGE_SIZE, PROT_READ | PROT_WRITE);
        mprotect(fbr->alloc_stack + (npages - 1) * PAGE_SIZE,
                 PAGE_SIZE,
                 PROT_READ | PROT_WRITE);
    }

    free(fbr->alloc_stack);
}

void
fiber_switch(Fiber *from, Fiber *to)
{
    assert(from && "Fiber cannot be NULL");
    assert(to && "Fiber cannot be NULL");

    if (from == to)
        return;

    assert(fiber_is_executing(from));
    assert(!fiber_is_executing(to));
    assert(fiber_is_alive(to));
    from->state &= ~FIBER_FS_EXECUTING;
    to->state |= FIBER_FS_EXECUTING;
    fiber_asm_switch(&from->regs, &to->regs);
}

void
fiber_push_return(Fiber *fbr, FiberFunc f, const void *args, size_t s)
{
    void *args_dest;
    fiber_reserve_return(fbr, f, &args_dest, s);
    memcpy(args_dest, args, s);
}

void
fiber_reserve_return(Fiber *fbr, FiberFunc f, void **args, size_t s)
{
    assert(fbr && "Fiber cannot be NULL");
    assert(!fiber_is_executing(fbr));

    char *sp = fbr->regs.sp;
    sp = (char *) STACK_ALIGN(sp);
    s = (s + STACK_ALIGNMENT - 1) & ~((size_t) STACK_ALIGNMENT - 1);

    sp -= s;
    *args = (void *) sp;

    sp -= sizeof fbr->regs.sp;
    *(void **) sp = fbr->regs.sp;

    sp -= sizeof *args;
    *(void **) sp = *args;

    sp -= sizeof(FiberFunc);
    *(FiberFunc *) sp = f;

    sp -= WORD_SIZE; // introduced to realign stack to 16 bytes

    assert(IS_STACK_ALIGNED(sp));

    sp -= sizeof(FiberFunc);
    *(FiberFunc *) sp = (FiberFunc) fiber_asm_invoke;

#ifdef FIBER_TARGET_64_AARCH
    sp -= WORD_SIZE;
    assert(IS_STACK_ALIGNED(sp));
#endif

    fbr->regs.sp = (void *) sp;
}

void
fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc f, void *args)
{
    assert(active && "Fiber cannot be NULL");
    assert(temp && "Fiber cannot be NULL");
    assert(fiber_is_executing(active));

    if (active == temp) {
        f(args);
    } else {
        assert(!fiber_is_executing(temp));
        temp->state |= FIBER_FS_EXECUTING;
        active->state &= ~FIBER_FS_EXECUTING;
        fiber_asm_exec_on_stack(temp->regs.sp, f, args);
        active->state |= FIBER_FS_EXECUTING;
        temp->state &= ~FIBER_FS_EXECUTING;
    }
}

static void
fiber_guard(void *argsp)
{
    FiberGuardArgs *args = (FiberGuardArgs *) argsp;
    args->fiber->state &= ~FIBER_FS_ALIVE;
    args->cleanup(args->fiber, args->arg);
#ifndef NDEBUG
    assert(0 && "ERROR: fiber cleanup returned");
#else
    fprintf(stderr, "ERROR: fiber cleanup returned\n");
#endif
    abort();
}

#ifdef FIBER_ASM_CHECK_ALIGNMENT
void
fiber_align_check_failed()
{
#ifndef NDEBUG
    assert(0 && "ERROR: fiber stack alignment check failed");
#else
    fprintf(stderr, "ERROR: fiber stack alignment check failed\n");
#endif
    abort();
}
#endif
