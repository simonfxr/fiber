#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <fiber/fiber.h>

#define STACK_SIZE (4096ul * 32)

typedef struct Thread Thread;

typedef struct Sched Sched;

struct Thread
{
    Sched *sched;
    Thread *prev, *next;
    Fiber fiber;
};

struct Sched
{
    Thread main_thread;
    Thread *running;
    Thread *done; /* only next ptr used */
    size_t fuel;
};

typedef struct
{
    Thread *thread;
    void (*entry)(void);
} ThreadArgs;

Thread *the_thread;

static Fiber *
thread_fiber(Thread *th)
{
    return &th->fiber;
}

static void
thread_switch(Thread *from, Thread *to)
{
    Sched *sched = from->sched;
    assert(sched->running == from);
    assert(the_thread == from);

    if (sched->fuel == 0)
        to = &sched->main_thread;
    else
        --sched->fuel;

    sched->running = to;
    the_thread = to;
    fiber_switch(thread_fiber(from), thread_fiber(to));
}

static void
yield(void)
{
    thread_switch(the_thread, the_thread->next);
}

static void
thread_destroy(Thread *th)
{
    fiber_destroy(thread_fiber(th));
    free(th);
}

static void
thread_end(Thread *th)
{
    Sched *sched = th->sched;
    Thread *next = th->next;

    th->prev->next = th->next;
    th->next->prev = th->prev;

    th->next = sched->done;
    th->prev = 0;
    sched->done = th;

    thread_switch(th, next);
}

static void
thread_exec(void *args0)
{
    ThreadArgs *args = (ThreadArgs *) args0;
    the_thread = args->thread;
    args->entry();
}

static void
fiber_cleanup(Fiber *fiber, void *arg)
{
    Thread *th = arg;
    thread_end(th);
}

static void
thread_start(Sched *sched, void (*func)(void))
{
    Thread *th = malloc(sizeof *th);
    th->sched = sched;
    fiber_alloc(&th->fiber, STACK_SIZE, fiber_cleanup, th, FIBER_FLAG_GUARD_LO);

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
sched_init(Sched *s)
{
    s->main_thread.prev = &s->main_thread;
    s->main_thread.next = &s->main_thread;
    s->main_thread.sched = s;
    fiber_init_toplevel(&s->main_thread.fiber);

    s->running = &s->main_thread;
    s->done = 0;

    the_thread = &s->main_thread;
}

static void
run_put_str(void *arg)
{
    const char *str = *(const char **) arg;
    puts(str);
}

static void
put_str(const char *str)
{
    fiber_exec_on(thread_fiber(the_thread),
                  &the_thread->sched->main_thread.fiber,
                  run_put_str,
                  (void *) &str);
}

static void
thread1(void)
{
    for (;;) {
        int x, y;
        put_str("thread1 running");
        yield();
    }
}

static void
thread2(void)
{
    for (;;) {
        put_str("thread2 running");
        yield();
    }
}

static void
thread3(void)
{
    int rounds = 10;
    while (rounds-- > 0) {
        put_str("thread3 running");
        yield();
    }

    put_str("thread3 exiting");
}

static void
execute(Sched *sched)
{
    while (sched->running->next != &sched->main_thread && sched->fuel > 0) {
        yield();

        if (sched->fuel == 0 && sched->running->prev != &sched->main_thread) {
            sched->running->prev->next = sched->done;
            sched->done = sched->running->next;
            sched->running->next = sched->running;
            sched->running->prev = sched->running;
        }

        while (sched->done != 0) {
            Thread *t = sched->done;
            sched->done = sched->done->next;
            printf("thread exiting: %p\n", t);
            thread_destroy(t);
        }
    }
}

int
main(void)
{
    Sched s;
    sched_init(&s);
    s.fuel = 100;

    thread_start(&s, thread1);
    thread_start(&s, thread2);
    thread_start(&s, thread3);

    execute(&s);

    return 0;
}
