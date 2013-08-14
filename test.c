#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "fiber.h"

#define STACK_SIZE 512ul

typedef struct Thread Thread;

typedef struct Sched Sched;

struct Thread {
    Thread *prev, *next;
    Fiber *fiber;
};

struct Sched {
    Thread main_thread;
    Fiber main_fiber;
    Thread *running;
    Thread *done; /* only next ptr used */
    size_t fuel;
};

typedef struct {
    void (*entry)(void);
} ThreadArgs;

static Sched *sched;

static Fiber *thread_fiber(Thread *t) {
    return t->fiber;
}

static void thread_switch(Thread *from, Thread *to) {
    assert(sched->running == from);
    
    if (sched->fuel == 0)
        to = &sched->main_thread;
    else
        --sched->fuel;

    sched->running = to;
    fiber_switch(thread_fiber(from), thread_fiber(to));
}

static void yield() {
    thread_switch(sched->running, sched->running->next);
}

static void thread_destroy(Thread *t) {
    fiber_free(thread_fiber(t));
    free(t);
}

static void thread_end() {
    Thread *t = sched->running;
    Thread *next = t->next;
    
    t->prev->next = t->next;
    t->next->prev = t->prev;
    
    t->next = sched->done;
    t->prev = 0;
    sched->done = t;

    thread_switch(t, next);
}

static void thread_exec(void *args0) {
    ThreadArgs *args = (ThreadArgs *) args0;
    args->entry();
    thread_end();
}

static void thread_start(void (*func)(void)) {
    Thread *t = malloc(sizeof *t);
    Fiber *fbr;
    fiber_alloc(&fbr, STACK_SIZE);
    t->fiber = fbr;

    ThreadArgs args;
    args.entry = func;
    fiber_push_return(fbr, thread_exec, &args, sizeof args);
    
    t->next = sched->running->next;
    t->prev = sched->running;
    t->next->prev = t;
    t->prev->next = t;
}

static void sched_init(Sched *s) {
    sched = s;
    s->main_thread.prev = &s->main_thread;
    s->main_thread.next = &s->main_thread;
    s->main_thread.fiber = &s->main_fiber;
    fiber_init_toplevel(&s->main_fiber);

    s->running = &s->main_thread;
    s->done = 0;
}

static void run_put_str(void *arg) {
    const char *str = * (const char **) arg;
    puts(str);
}

static void put_str(const char *str) {
    fiber_exec_on(thread_fiber(sched->running),
                  &sched->main_fiber, run_put_str, &str, sizeof str);
}

static void thread1() {
    for (;;) {
        put_str("thread1 running");
        yield();
    }
}

static void thread2() {
    for (;;) {
        put_str("thread2 running");
        yield();
    }
}

static void thread3() {
    int rounds = 10;
    while (rounds --> 0) {
        put_str("thread3 running");
        yield();
    }
    
    put_str("thread3 exiting");
}

int main(void) {

    Sched s;
    sched_init(&s);
    s.fuel = 100;

    thread_start(thread1);
    thread_start(thread2);
    thread_start(thread3);

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
    
    return 0;
}
