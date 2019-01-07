#include <fiber/fiber.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void
fiber_cleanup(Fiber *fiber, void *args)
{
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
    fiber_switch(args->self, args->caller);
}

int
main()
{
    Fiber toplevel;
    fiber_init_toplevel(&toplevel);
    Fiber fiber;
    fiber_alloc(&fiber, 4096 * 4, fiber_cleanup, NULL, true);
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