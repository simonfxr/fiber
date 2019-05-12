#include <fiber/fiber.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE ((size_t) 1024 * 16)

static void
fiber_cleanup(Fiber *fiber, void *args)
{
    (void) fiber;
    (void) args;
    assert(0 && "fiber_cleanup should not be called");
    abort();
}

typedef struct
{
    Fiber *self;
    Fiber *caller;
} FiberArgs;

static void
run_put_str(void *arg)
{
    puts((const char *) arg);
}

static void
put_str(Fiber *active, Fiber *at, const char *arg)
{
    fiber_exec_on(active, at, run_put_str, (void *) arg);
}

static void
fiber_entry(void *argsp)
{
    FiberArgs *args = (FiberArgs *) argsp;
    puts("fiber_entry()");
    fiber_switch(args->self, args->caller);

    put_str(args->self, args->caller, "some string");
    put_str(args->self, args->caller, "some other string");

    puts("again fiber_entry()");
    char *msg;
    size_t sz = 64 * 1024;
    fiber_reserve_return(args->caller, run_put_str, (void **) &msg, sz);
    memset(msg, 0, sz);
    strcpy(msg, "Pushed onto toplevel fiber");
    fiber_switch(args->self, args->caller);
}

int
main()
{
    Fiber toplevel;
    fiber_init_toplevel(&toplevel);
    Fiber fiber;
    (void) fiber_alloc(&fiber,
                       STACK_SIZE,
                       fiber_cleanup,
                       NULL,
                       FIBER_FLAG_GUARD_LO | FIBER_FLAG_GUARD_HI);
    FiberArgs *args;
    fiber_reserve_return(&fiber, fiber_entry, (void **) &args, sizeof *args);
    args->self = &fiber;
    args->caller = &toplevel;
    fiber_switch(&toplevel, &fiber);
    puts("in main()");
    fiber_switch(&toplevel, &fiber);
    fiber_destroy(&fiber);
    return 0;
}
