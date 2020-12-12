#include <fiber/fiber.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    Fiber *caller;
    Fiber *self;
    const char *str;
} Args;

/* invoked if coroutine is called after it finished */
void
guard(Fiber *self, void *null)
{
    (void) self;
    (void) null;
    abort();
}

/* entry point */
void
coroutine(void *args0)
{
    Args *args = (Args *) args0;
    printf("From coroutine: %s\n", args->str);
    fiber_switch(args->self, args->caller);
}

int
main()
{
    Fiber main_fiber;
    fiber_init_toplevel(&main_fiber, NULL);
    Fiber crt;
    /* allocate a stack of 16kb, additionally add an unmapped page to detect
     * overflows */
    (void) fiber_alloc(&crt, 1024 * 16, guard, NULL, FIBER_FLAG_GUARD_LO);
    Args args;
    args.caller = &main_fiber;
    args.self = &crt;
    args.str = "Hello";
    /* push a new return stack frame, defines the entry point of our coroutine
     */
    fiber_push_return(&crt, coroutine, &args, sizeof args);
    /* arguments are copied into the coroutine, its safe to destroy them here */
    memset(&args, 0, sizeof args);
    /* run our coroutine */
    fiber_switch(&main_fiber, &crt);
    fiber_destroy(&crt);
    /* main_fiber does not have to be destroyed */
    return 0;
}
