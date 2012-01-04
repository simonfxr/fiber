#include "fiber.h"
#include "fiber_asm.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef unsigned char byte;
typedef uintptr_t uptr;

static const size_t WORD_SIZE = sizeof (void *);
static const size_t STACK_ALIGNMENT = WORD_SIZE * 2;

static const size_t COOKIE = 0xFEABCAC0;

#define ASSERT(a) assert(a)

#define UNUSED(a) ((void) (a))

#define STACK_IS_ALIGNED(x) (((x) & (STACK_ALIGNMENT - 1)) == 0)

static void fiber_guard(const void *);

Fiber *fiber_init(void *stack, size_t sz) {
    byte *top = &((byte *) stack)[sz];
    Fiber *fbr = (Fiber *) ((uptr) (top - sizeof (Fiber)) & ~(WORD_SIZE - 1));
    if ((void *) fbr < stack)
        return NULL;

    fbr->cookie = COOKIE;
    fbr->state = 0;
    fbr->size = sz;
    fbr->offset_to_stack = top - (byte *) (fbr + 1);
    fbr->stack_ptr = (void *) (((uptr) fbr - STACK_ALIGNMENT - 1) & ~(STACK_ALIGNMENT - 1));

    if (fbr->stack_ptr < stack)
        return NULL;

    fiber_push_return(fbr, fiber_guard, &fbr, sizeof fbr);
    
    return fbr;
}

void fiber_init_toplevel(Fiber *fbr) {
    fbr->cookie = COOKIE;
    fbr->state = FS_TOPLEVEL | FS_EXECUTING;
    fbr->size = sizeof (Fiber);
    fbr->offset_to_stack = 0;
    fbr->stack_ptr = 0;
}

void *fiber_stack(Fiber *fbr) {
    ASSERT(fbr != 0);
    return (byte *) (fbr + 1) + fbr->offset_to_stack - fbr->size;
}

int fiber_is_toplevel(Fiber *fbr) {
    ASSERT(fbr != 0);
    return (fbr->state & FS_TOPLEVEL) != 0;
}

void fiber_switch(Fiber *from, Fiber *to) {
    ASSERT(from != 0);
    ASSERT(to != 0);
    if (from == to)
        return;

    ASSERT((from->state & FS_EXECUTING) != 0);
    ASSERT((to->state & FS_EXECUTING) == 0);
    from->state &= ~ FS_EXECUTING;
    to->state |= FS_EXECUTING;

    fiber_asm_switch(&from->stack_ptr, to->stack_ptr);
}

void fiber_push_return(Fiber *fbr, FiberFunc f, const void *args, size_t s) {
    ASSERT(fbr != 0);
    ASSERT(f != 0);
    ASSERT((fbr->state & FS_EXECUTING) == 0);
    ASSERT(s > 0 || args != 0);

    size_t aligned_size = (s + STACK_ALIGNMENT - 1) & ~(STACK_ALIGNMENT - 1);

    byte *stack_ptr = (byte *) fbr->stack_ptr;
    stack_ptr -= aligned_size;
    memcpy(stack_ptr, args, s);
    
    stack_ptr -= sizeof (void *);
    * (void **) stack_ptr = f;
    
    stack_ptr -= sizeof (size_t);
    * (size_t *) stack_ptr = aligned_size;
    
    fbr->stack_ptr = stack_ptr;
    fiber_asm_push_invoker(&fbr->stack_ptr);
}

void fiber_exec_on(Fiber *active, Fiber *temp, FiberFunc f, const void *args, size_t s) {
    UNUSED(s);
    ASSERT(active != 0);
    ASSERT(temp != 0);
    ASSERT((active->state & FS_EXECUTING) != 0);
    if (active == temp) {
        f(args);
    } else {
        ASSERT((temp->state & FS_EXECUTING) == 0);
        temp->state |= FS_EXECUTING;
        active->state &= ~ FS_EXECUTING;
        fiber_asm_exec_on_stack(temp->stack_ptr, f, args);
        active->state |= FS_EXECUTING;
        temp->state &= ~ FS_EXECUTING;
    }
}

static void fiber_guard(const void *args) {
    UNUSED(args);
    abort(); // cannot continue
}
