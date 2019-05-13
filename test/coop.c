#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef FIBER_AMALGAMATED
#    include <fiber/fiber.h>
#endif

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

static Thread *the_thread;

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
    assert(sched->running == from);
    assert(the_thread == from);

    if (sched->fuel > 0)
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
    printf("[Scheduler] thread exited: %d\n", args->thread->id);
    fiber_destroy(&args->thread->fiber);
    free(args->thread);
}

static void
thread_guard(Fiber *self, void *arg)
{
    Thread *th = (Thread *) arg;
    assert(the_thread);
    assert(th == the_thread);
    assert(self == &th->fiber);
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
sched_init(Sched *s)
{
    s->main_thread.prev = &s->main_thread;
    s->main_thread.next = &s->main_thread;
    s->main_thread.sched = s;
    fiber_init_toplevel(&s->main_thread.fiber);

    s->running = &s->main_thread;
    s->done = 0;
    s->next_id = 1;
    s->shutdown_signal = false;
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
                  (void **) &str);
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
    printf("[Worker %d] started, work=%d\n", my_id, work);
    while (!shutting_down() && work-- > 0) {
        h ^= (unsigned) work;
        h = (h << 13) | (h >> 19);
        h *= 1337;
        yield();
    }
    printf("[Worker %d] exiting, result: %u\n", my_id, h);
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
            printf("[Thread 2] received ping: %d\n", tok);
            message = MsgNone;
            break;
        case MsgFork:
            printf("[Thread 2] forking worker: %d\n", tok);
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
            printf("[Scheduler] sending shutdown signal\n");
        }
    }

    printf("[Scheduler] all threads exited\n");
    the_thread = NULL;
}

int
main(void)
{
    Sched s;
    sched_init(&s);
    s.fuel = 1000;

    thread_start(&s, thread1);
    thread_start(&s, thread2);
    thread_start(&s, thread3);

    execute(&s);

    return 0;
}
