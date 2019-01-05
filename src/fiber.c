#include <fiber/fiber.h>

#include "fiber_asm.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <stdio.h>

#ifdef HU_BITS_32
#define WORD_SIZE ((size_t) 4)
#define STACK_ALIGNMENT ((uintptr_t) 16)
#define RED_ZONE ((size_t) 0)
#else
#define WORD_SIZE ((size_t) 8)
#define STACK_ALIGNMENT ((uintptr_t) 16)
#define RED_ZONE ((size_t) 128)
#endif

typedef unsigned char byte;

#define UNUSED(a) ((void) (a))

#define IS_STACK_ALIGNED(x) (((uintptr_t)(x) & (STACK_ALIGNMENT - 1)) == 0)
#define STACK_ALIGN(x) ((uintptr_t)(x) & ~(STACK_ALIGNMENT - 1))

static void
fiber_guard(void *);

Fiber *
fiber_init(Fiber *fbr, void *stack, size_t stack_size)
{
    fbr->stack = stack;
    fbr->stack_size = stack_size;

    memset(&fbr->regs, 0, sizeof fbr->regs);
    uintptr_t sp = (uintptr_t)((char *) stack + stack_size - WORD_SIZE);
    sp &= ~(STACK_ALIGNMENT - 1);
    fbr->regs.sp = (void *) sp;
    void **fbr_dest;
    fiber_reserve_return(fbr, fiber_guard, (void **) &fbr_dest, sizeof fbr);
    *fbr_dest = fbr;
    fbr->state = FIBER_FS_ALIVE;
    return fbr;
}

void
fiber_init_toplevel(Fiber *fbr)
{
    fbr->stack = 0;
    fbr->stack_size = 0;
    memset(&fbr->regs, 0, sizeof fbr->regs);
    fbr->state = FIBER_FS_ALIVE | FIBER_FS_TOPLEVEL | FIBER_FS_EXECUTING;
}

static const size_t PAGE_SIZE = 4096;

int
fiber_alloc(Fiber *fbr, size_t size, bool use_guard_pages)
{
    char *stack;
    size_t stack_size;

    if (!use_guard_pages) {
        stack_size = size;
        stack = malloc(stack_size);
        if (!stack)
            return 0;
    } else {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        npages += 2; // guard pages ;
        stack_size = (npages - 2) * PAGE_SIZE;
        stack = (char *) mmap(0,
                              npages * PAGE_SIZE,
                              PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS,
                              0,
                              0);
        if (stack == (char *) (intptr_t) -1)
            return 0;

        if (mprotect(stack, PAGE_SIZE, PROT_NONE) == -1)
            goto fail;

        if (mprotect(stack + (npages - 1) * PAGE_SIZE, PAGE_SIZE, PROT_NONE) ==
            -1)
            goto fail;

        stack += PAGE_SIZE;
    }

    fiber_init(fbr, stack, stack_size);
    if (use_guard_pages)
        fbr->state |= FIBER_FS_HAS_GUARD_PAGES;

    return 1;

fail:
    munmap(stack, stack_size + 2 * PAGE_SIZE);
    return 0;
}

void
fiber_destroy(Fiber *fbr)
{
    assert(!fiber_is_executing(fbr));
    assert(!fiber_is_toplevel(fbr));

    if (fbr->state & FIBER_FS_HAS_GUARD_PAGES) {
        munmap(fbr->stack - PAGE_SIZE, fbr->stack_size + 2 * PAGE_SIZE);
    } else {
        free(fbr->stack);
    }
}

void
fiber_switch(Fiber *from, Fiber *to)
{
    assert(from != 0);
    assert(to != 0);

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
    assert(fbr != 0);
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

#ifdef HU_BITS_64
    sp -= WORD_SIZE;
#endif

    assert(IS_STACK_ALIGNED(sp));

    sp -= sizeof(FiberFunc);
    *(FiberFunc *) sp = (FiberFunc) fiber_asm_invoke;

    fbr->regs.sp = (void *) sp;
}

void
fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc f, void *args, size_t s)
{
    UNUSED(s);
    assert(active != 0);
    assert(temp != 0);
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
fiber_guard(void *args)
{
    UNUSED(args);
    abort(); // cannot continue
}
