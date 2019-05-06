#include <fiber/fiber.h>

#include "fiber_asm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HU_OS_POSIX_P
#include <sys/mman.h>
#elif HU_OS_WINDOWS_P
#include <Windows.h>
#endif

#define UNUSED(a) ((void) (a))

#ifndef FIBER_STACK_ALIGNMENT
static const size_t STACK_ALIGNMENT = FIBER_DEFAULT_STACK_ALIGNMENT;
#else
static const size_t STACK_ALIGNMENT = FIBER_STACK_ALIGNMENT;
#endif

static const size_t ARG_ALIGNMENT = 8;
static const size_t WORD_SIZE = sizeof(void *);
static const size_t PAGE_SIZE = 4096;

static inline void *
stack_align_n(void *sp, size_t n)
{
    return (void *) ((uintptr_t) sp & ~(uintptr_t)(n - 1));
}

static inline void *
stack_align(void *sp)
{
    return stack_align_n(sp, STACK_ALIGNMENT);
}

static inline bool
is_stack_aligned(void *sp)
{
    return ((uintptr_t) sp & (STACK_ALIGNMENT - 1)) == 0;
}

static inline void
push(char **sp, void *val)
{
    *sp -= WORD_SIZE;
    *(void **) *sp = val;
}

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

static void *
alloc_pages(size_t npages)
{
#if HU_OS_POSIX_P
    void *ret = NULL;
    if (posix_memalign(&ret, PAGE_SIZE, npages * PAGE_SIZE))
        return NULL;
    return ret;
#elif HU_OS_WINDOWS_P
    return _aligned_malloc(npages * PAGE_SIZE, PAGE_SIZE);
#else
#error "BUG: platform not properly handled"
#endif
}

static void
free_pages(void *p)
{
#if HU_OS_POSIX_P
    free(p);
#elif HU_OS_WINDOWS_P
    _aligned_free(p);
#else
#error "BUG: platform not properly handled"
#endif
}

static bool
protect_page(void *p, bool rw)
{
#if HU_OS_POSIX_P
    return mprotect(p, PAGE_SIZE, rw ? PROT_READ | PROT_WRITE : PROT_NONE) == 0;
#elif HU_OS_WINDOWS_P
    DWORD old_protect;
    return VirtualProtect(
             p, PAGE_SIZE, rw ? PAGE_READWRITE : PAGE_NOACCESS, &old_protect) !=
           0;
#else
#error "BUG: platform not properly handled"
#endif
}

bool
fiber_alloc(Fiber *fbr,
            size_t size,
            FiberCleanupFunc cleanup,
            void *arg,
            FiberFlags flags)
{
    flags &= FIBER_FLAG_GUARD_LO | FIBER_FLAG_GUARD_HI;
    assert(fbr && "Fiber can not be null");
    fbr->stack_size = size;
    const size_t stack_size = size;

    if (!flags) {
        fbr->alloc_stack = fbr->stack = malloc(stack_size);
        if (!fbr->alloc_stack)
            return false;
    } else {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        if (flags & FIBER_FLAG_GUARD_LO)
            ++npages;
        if (flags & FIBER_FLAG_GUARD_HI)
            ++npages;
        fbr->alloc_stack = alloc_pages(npages);
        if (!fbr->alloc_stack)
            return false;

        if (flags & FIBER_FLAG_GUARD_LO)
            if (!protect_page(fbr->alloc_stack, false))
                goto fail;

        if (flags & FIBER_FLAG_GUARD_HI)
            if (!protect_page(
                  (char *) fbr->alloc_stack + (npages - 1) * PAGE_SIZE, false))
                goto fail;
        if (flags & FIBER_FLAG_GUARD_LO)
            fbr->stack = (char *) fbr->alloc_stack + PAGE_SIZE;
        else
            fbr->stack = fbr->alloc_stack;
    }

    fbr->state = flags;
    fiber_init_(fbr, cleanup, arg);
    return true;

fail:
    free_pages(fbr->alloc_stack);
    return false;
}

void
fiber_destroy(Fiber *fbr)
{
    assert(!fiber_is_executing(fbr));
    assert(!fiber_is_toplevel(fbr));

    if (!fbr->alloc_stack)
        return;

    if (fbr->state &
        (FIBER_FS_HAS_HI_GUARD_PAGE | FIBER_FS_HAS_LO_GUARD_PAGE)) {
        size_t npages = (fbr->stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
        if (fbr->state & FIBER_FS_HAS_LO_GUARD_PAGE) {
            ++npages;
            protect_page(fbr->alloc_stack, true);
        }

        if (fbr->state & FIBER_FS_HAS_HI_GUARD_PAGE) {
            protect_page((char *) fbr->alloc_stack + npages * PAGE_SIZE, true);
        }

        free_pages(fbr->alloc_stack);
    } else {
        free(fbr->alloc_stack);
    }

    fbr->stack = NULL;
    fbr->stack_size = 0;
    fbr->regs.sp = NULL;
    fbr->alloc_stack = NULL;
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

#if hu_has_attribute(weak)
#define HAVE_probe_stack_weak_dummy
__attribute__((weak)) void
_probe_stack_weak_dummy(volatile char *sp, size_t sz);

__attribute__((weak)) void
_probe_stack_weak_dummy(volatile char *sp, size_t sz)
{
    (void) sp;
    (void) sz;
}
#endif

HU_NOINLINE
static void
probe_stack(volatile char *sp0, size_t sz)
{
    volatile char *sp = sp0;
#if HU_COMP_GNUC_P
    __asm__ __volatile__("" : : "r"(sp) : "memory");
#endif

    size_t i = 0;
    while (i < sz) {
        *(volatile uintptr_t *) sp |= (uintptr_t) 0;
        i += PAGE_SIZE;
        sp -= PAGE_SIZE;
    }

#ifdef HAVE_probe_stack_weak_dummy
    _probe_stack_weak_dummy(sp0, sz);
#endif
}

void
fiber_reserve_return(Fiber *fbr, FiberFunc f, void **args, size_t s)
{
    assert(fbr && "Fiber cannot be NULL");
    assert(!fiber_is_executing(fbr));

    char *sp = fbr->regs.sp;
    size_t arg_align =
      ARG_ALIGNMENT > STACK_ALIGNMENT ? ARG_ALIGNMENT : STACK_ALIGNMENT;
    sp = stack_align_n(sp - s, arg_align);
    *args = sp;

    if (hu_unlikely(s > PAGE_SIZE - 100))
        probe_stack(sp, s);

    assert(is_stack_aligned(sp) && "1");

#ifdef FIBER_HAVE_LR
    push(&sp, fbr->regs.lr);
#endif

    push(&sp, fbr->regs.sp);
    push(&sp, f);
    push(&sp, *args);

#ifndef FIBER_HAVE_LR
    if (STACK_ALIGNMENT >= 8)
        sp -= WORD_SIZE; // introduced to realign stack to 16 bytes
#endif
    assert(is_stack_aligned(sp) && "2");

#ifdef FIBER_HAVE_LR
    fbr->regs.lr = fiber_asm_invoke;
#else
    push(&sp, fiber_asm_invoke);
#endif

    fbr->regs.sp = (void *) sp;
#undef PUSH
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
        fiber_asm_exec_on_stack(args, f, temp->regs.sp);
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
fiber_align_check_failed(void)
{
#ifndef NDEBUG
    assert(0 && "ERROR: fiber stack alignment check failed");
#else
    fprintf(stderr, "ERROR: fiber stack alignment check failed\n");
#endif
    abort();
}
#endif
