#define _GNU_SOURCE 1

#include "fiber.h"
#include "fiber_asm.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>

#ifdef FIBER_BITS32
#  define WORD_SIZE ((size_t) 4)
#  define STACK_ALIGNMENT ((uptr) 4)
#else
#  define WORD_SIZE ((size_t) 8)
#  define STACK_ALIGNMENT ((uptr) 16)
#endif

typedef unsigned char byte;
typedef uintptr_t uptr;

#define ASSERT(a) assert(a)

#define UNUSED(a) ((void) (a))

#define IS_STACK_ALIGNED(x) (((uintptr_t) (x) & (STACK_ALIGNMENT - 1)) == 0)

static void fiber_guard(void *);

Fiber *fiber_init(Fiber *fbr, void *stack, size_t stack_size) {
    fbr->stack = stack;
    fbr->stack_size = stack_size;

    memset(&fbr->regs, 0, sizeof fbr->regs);
    uptr sp = (uptr) ((char *) stack + stack_size - WORD_SIZE);
    sp &= ~ (STACK_ALIGNMENT - 1);
    fbr->regs.sp = (void *) sp;
    void **fbr_dest;
    fiber_reserve_return(fbr, fiber_guard, (void **) &fbr_dest, sizeof fbr);
    *fbr_dest = fbr; 
    fbr->state = FS_ALIVE;
    return fbr;
}

void fiber_init_toplevel(Fiber *fbr) {
    fbr->stack = 0;
    fbr->stack_size = 0;
    memset(&fbr->regs, 0, sizeof fbr->regs);
    fbr->state = FS_ALIVE | FS_TOPLEVEL | FS_EXECUTING;
}

void fiber_alloc(Fiber **fbrp, size_t size) {
    const size_t page_size = 4096;
    size_t npages = (size + page_size - 1) / page_size;
    npages += 2; // guard pages ;
    char *stack = (char *) mmap(0, npages * page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (stack == (char *) (intptr_t) - 1) {
        *fbrp = 0;
        return;
    }

    if (mprotect(stack, page_size, PROT_NONE) == -1) {
        *fbrp = 0;
        return;
    }

    if (mprotect(stack + (npages - 1) * page_size, page_size, PROT_NONE) == -1) {
        *fbrp = 0;
        return;
    }

    Fiber *fbr = malloc(sizeof *fbr);
    fiber_init(fbr, stack + page_size, (npages - 2) * page_size);
    *fbrp = fbr;
}

void fiber_free(Fiber *fbr) {
    munmap(fbr->stack, fbr->stack_size + 2 * 4096);
    free(fbr);
}

void fiber_switch(Fiber *from, Fiber *to) {
    ASSERT(from != 0);
    ASSERT(to != 0);
    
    if (from == to)
        return;

    ASSERT(fiber_is_executing(from));
    ASSERT(!fiber_is_executing(to));
    ASSERT(fiber_is_alive(to));
    from->state &= ~ FS_EXECUTING;
    to->state |= FS_EXECUTING;
    fiber_asm_switch(&from->regs, &to->regs);
}

void fiber_push_return(Fiber *fbr, FiberFunc f, const void *args, size_t s) {
    void *args_dest;
    fiber_reserve_return(fbr, f, &args_dest, s);
    memcpy(args_dest, args, s);
}

void fiber_reserve_return(Fiber *fbr, FiberFunc f, void **args, size_t s) {
    ASSERT(fbr != 0);
    ASSERT(!fiber_is_executing(fbr));

    char *sp = fbr->regs.sp;
    sp = (char *) ((uptr) sp & ~ (STACK_ALIGNMENT - 1)); // align stack
    s = (s + STACK_ALIGNMENT - 1) & ~ ((size_t) STACK_ALIGNMENT - 1);

    sp -= s;
    *args = (void *) sp;
    
    sp -= sizeof fbr->regs.sp;    
    *(void **)sp = fbr->regs.sp;

    sp -= sizeof *args;
    *(void **)sp = *args;

    sp -= sizeof (FiberFunc);
    *(FiberFunc *)sp = f;

    sp -= WORD_SIZE; // push junk (for alignment)

    sp -= sizeof (FiberFunc);
    *(FiberFunc *)sp = (FiberFunc) fiber_asm_invoke;

    fbr->regs.sp = (void *)sp;
}


void fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc f, void *args, size_t s) {
    UNUSED(s);
    ASSERT(active != 0);
    ASSERT(temp != 0);
    ASSERT(fiber_is_executing(active));
    
    if (active == temp) {
        f(args);
    } else {
        ASSERT(!fiber_is_executing(temp));
        temp->state |= FS_EXECUTING;
        active->state &= ~ FS_EXECUTING;
        fiber_asm_exec_on_stack(temp->regs.sp, f, args);
        active->state |= FS_EXECUTING;
        temp->state &= ~ FS_EXECUTING;
    }
}

static void fiber_guard(void *args) {
    UNUSED(args);
    abort(); // cannot continue
}

void asm_assert_fail() {
    abort();
}
