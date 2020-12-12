#ifndef FIBER_AMALGAMATED
#    include <fiber/fiber.h>
#endif

#include "test_pre.h"

#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE ((size_t) 16 * 1024)

typedef struct Thread Thread;

typedef struct Sched Sched;

struct Thread
{
    Sched *sched;
    Thread *prev, *next;
    Fiber fiber;
    int id;
};

struct Sched
{
    Thread main_thread;
    Fiber trampoline_fiber;

    Thread *previous;

    Thread *running;
    Thread *done; /* only next ptr used */
    size_t fuel;
    int next_id;
    bool shutdown_signal;
};

typedef struct
{
    Thread *thread;
    void (*entry)(void);
} ThreadArgs;

typedef struct
{
    Thread *thread;
} CleanupArgs;

static Sched the_sched;

#define the_thread (the_sched.running)

static bool
shutting_down(void)
{
    return the_thread->sched->shutdown_signal;
}

static Fiber *
thread_fiber(Thread *th)
{
    return &th->fiber;
}

static void
thread_switch(Thread *from, Thread *to)
{
    Sched *sched = from->sched;
    require(sched->running == from);
    require(the_thread == from);

    if (sched->fuel > 0)
        --sched->fuel;

    sched->running = to;
    sched->previous = from;
    the_thread = to;
    fiber_switch(thread_fiber(from), &sched->trampoline_fiber);
}

static void
yield(void)
{
    thread_switch(the_thread, the_thread->next);
}

static void
thread_exec(void *args0)
{
    ThreadArgs *args = (ThreadArgs *) args0;
    the_thread = args->thread;
    args->entry();
}

static void
thread_cleanup_cont(void *args0)
{
    CleanupArgs *args = (CleanupArgs *) args0;
    fprintf(out, "[Scheduler] thread exited: %d\n", args->thread->id);
    fiber_destroy(&args->thread->fiber);
    free(args->thread);
}

static void
thread_guard(Fiber *self, void *arg)
{
    Thread *th = (Thread *) arg;
    require(the_thread);
    require(th == the_thread);
    require(self == &th->fiber);
    (void) self;

    Sched *sched = th->sched;
    Thread *sched_thread = &sched->main_thread;
    CleanupArgs *args;

    fiber_reserve_return(
      &sched_thread->fiber, thread_cleanup_cont, (void **) &args, sizeof args);
    args->thread = th;

    Thread *next = th->next;
    // unlink ourself
    th->prev->next = th->next;
    th->next->prev = th->prev;
    thread_switch(th, next);
    // does not return here
    abort();
}

static void
thread_start(Sched *sched, void (*func)(void))
{
    Thread *th = hu_cxx_static_cast(Thread *, malloc(sizeof *th));
    th->sched = sched;
    th->id = sched->next_id++;
    (void) fiber_alloc(
      &th->fiber, STACK_SIZE, thread_guard, th, FIBER_FLAG_GUARD_LO);

    ThreadArgs args;
    args.thread = th;
    args.entry = func;
    fiber_push_return(&th->fiber, thread_exec, &args, sizeof args);

    th->next = sched->running->next;
    th->prev = sched->running;
    th->next->prev = th;
    th->prev->next = th;
}

static void
trampoline_bottom(Fiber *fiber, void *null)
{
    (void) fiber;
    (void) null;
    abort();
}

static inline char *
fiber_stack_base(Fiber *fbr)
{
    return fbr->_regs.base;
}

static void
trampoline_run(void *null)
{
    (void) null;
    Sched *sched = the_thread->sched;

    size_t word_size = sizeof(void *);

    Fiber *main = thread_fiber(&sched->main_thread);
    char *e_stack_base = fiber_stack_base(main);
    main->_stack = malloc(STACK_SIZE);
    main->_stack_size = STACK_SIZE;
    fiber_switch(&sched->trampoline_fiber, thread_fiber(&sched->main_thread));

    for (;;) {
        Thread *from_th = sched->previous;
        Thread *to_th = sched->running;

        Fiber *from = thread_fiber(from_th);
        Fiber *to = thread_fiber(to_th);
        /* fprintf(out, "from(%p) -> to(%p)\n", from, to); */
        /* fflush(out); */

        char *s_stack, *s_stack_lim;
        size_t sz;

        sz = fiber_stack_used_size(from);
        require(sz > 0);
        from->_regs.base =
          fiber_stack_compute_base(from->_stack, from->_stack_size);
        memcpy(fiber_stack_base(from) - sz, e_stack_base - sz, sz);

        sz = fiber_stack_used_size(to);
        require(sz > 0);
        memcpy(e_stack_base - sz, fiber_stack_base(to) - sz, sz);
        to->_regs.base = e_stack_base;

        fiber_switch(&sched->trampoline_fiber, to);
    }
}

static void
sched_init(Sched *s)
{
    s->main_thread.prev = &s->main_thread;
    s->main_thread.next = &s->main_thread;
    s->main_thread.sched = s;

    the_thread = &s->main_thread;

    fiber_init_toplevel(&s->main_thread.fiber);

    (void) fiber_alloc(&s->trampoline_fiber,
                       STACK_SIZE,
                       trampoline_bottom,
                       NULL,
                       FIBER_FLAG_GUARD_LO);

    fiber_push_return(&s->trampoline_fiber, trampoline_run, NULL, 0);
    fiber_switch(&s->main_thread.fiber, &s->trampoline_fiber);

    s->running = &s->main_thread;
    s->done = 0;
    s->next_id = 1;
    s->shutdown_signal = false;
}

static void
put_str(const char *str)
{
    println(str);
}

typedef enum Message
{
    MsgNone,
    MsgPing,
    MsgFork,
} Message;

static int next_worker_id;
static Message message;

static void
worker(void)
{
    int my_id = next_worker_id++;
    int work = 13 + ((my_id * 17) % 11);
    unsigned h = 42;
    fprintf(out, "[Worker %d] started, work=%d\n", my_id, work);
    while (!shutting_down() && work-- > 0) {
        h ^= (unsigned) work;
        h = (h << 13) | (h >> 19);
        h *= 1337;
        yield();
    }
    fprintf(out, "[Worker %d] exiting, result: %u\n", my_id, h);
}

static void
thread1(void)
{
    put_str("[Thread 1] started");
    int tok = 0;
    while (!shutting_down()) {
        char str[64];
        snprintf(str, sizeof str, "[Thread 1] running: %d", tok++);
        put_str(str);
        if (tok % 30 == 0)
            message = MsgFork;
        else if (tok % 6 == 0)
            message = MsgPing;
        yield();
    }
    put_str("[Thread 1] exiting");
}

static void
thread2(void)
{
    put_str("[Thread 2] started");
    int tok = 0;
    while (!shutting_down()) {
        switch (message) {
        case MsgNone:
            break;
        case MsgPing:
            fprintf(out, "[Thread 2] received ping: %d\n", tok);
            message = MsgNone;
            break;
        case MsgFork:
            fprintf(out, "[Thread 2] forking worker: %d\n", tok);
            thread_start(the_thread->sched, worker);
            message = MsgNone;
            break;
        }
        char str[64];
        snprintf(str, sizeof str, "[Thread 2] running: %d", tok++);
        put_str(str);
        yield();
    }
    put_str("[Thread 2] exiting");
}

static void
thread3(void)
{
    put_str("[Thread 3] started");
    int tok = 0;
    int rounds = 10;
    while (!shutting_down() && rounds-- > 0) {
        char str[64];
        snprintf(str, sizeof str, "[Thread 3] running: %d", tok++);
        put_str(str);
        yield();
    }

    put_str("[Thread 3] exiting");
}

static void
execute(Sched *sched)
{
    the_thread = &sched->main_thread;
    while (sched->running->next != &sched->main_thread) {
        yield();

        if (sched->fuel == 0 && !sched->shutdown_signal) {
            sched->shutdown_signal = true;
            println("[Scheduler] sending shutdown signal");
        }
    }

    println("[Scheduler] all threads exited");
    the_thread = NULL;
}

int
main(int argc, char *argv[])
{
    test_main_begin(&argc, &argv);
    Sched *s = &the_sched;
    sched_init(s);
    s->fuel = 1000;

    thread_start(s, thread1);
    thread_start(s, thread2);
    thread_start(s, thread3);

    execute(s);

    test_main_end();
    return 0;
}
