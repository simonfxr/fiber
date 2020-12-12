#define __STDC_FORMAT_MACROS 1

#ifndef FIBER_AMALGAMATED
#    include <fiber/fiber.h>
#endif

#include "test_pre.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE ((size_t) 16 * 1024)

typedef union
{
    uint32_t u32;
    uint64_t u64;
    int32_t i32;
    int64_t i64;
    float f32;
    double f64;
    void *ptr;
} Value;

typedef struct
{
    Fiber fiber;
    Value result;
    const char *name;
    int id;
    bool done;
    bool should_finish;
} Generator;

typedef struct
{
    void (*entry)(void *);
    void *args;
} GeneratorArgs;

typedef struct
{
    Fiber *fiber;
    Fiber *caller;
    Generator *gen;
} Context;

static Context active_context;
static int gen_next_id;

static void
zero_value(Value *v)
{
    memset(v, 0, sizeof *v);
}

static void
fiber_guard(Fiber *self, void *null)
{
    (void) null;
    fprintf(stderr, "fiber_guard(fiber=%p) called, aborting\n", self);
    abort();
}

static void
gen_start(void *args0)
{
    GeneratorArgs *args = (GeneratorArgs *) args0;
    Generator *gen = active_context.gen;
    require(gen);
    fprintf(out, "[Generator[%d] %s] STARTING\n", gen->id, gen->name);
    args->entry(args->args);
    gen->done = true;
    fprintf(out, "[Generator[%d] %s] FINISH\n", gen->id, gen->name);
    free(args->args);
    fiber_switch(active_context.fiber, active_context.caller);
    // noreturn
    abort();
}

static Generator *
gen_new(const char *name, void (*f)(void *), void *args, size_t args_size)
{
    Generator *gen = hu_cxx_static_cast(Generator *, calloc(1, sizeof *gen));
    zero_value(&gen->result);
    gen->name = name;
    gen->id = gen_next_id++;
    gen->done = false;
    gen->should_finish = false;

    GeneratorArgs gen_args;
    gen_args.entry = f;
    gen_args.args = calloc(1, args_size);
    memcpy(gen_args.args, args, args_size);

    (void) fiber_alloc(
      &gen->fiber, STACK_SIZE, fiber_guard, NULL, FIBER_FLAG_GUARD_LO);
    fiber_push_return(&gen->fiber, gen_start, &gen_args, sizeof gen_args);
    return gen;
}

static void
gen_init_toplevel(Fiber *fiber, void *addr)
{
    require(!active_context.fiber);
    fiber_init_toplevel(fiber, addr);
    active_context.fiber = fiber;
    active_context.gen = NULL;
    active_context.caller = NULL;
}

static bool
gen_next(Generator *gen, Value *v)
{
    zero_value(v);
    if (gen->done)
        return false;
    Context saved = active_context;
    active_context.fiber = &gen->fiber;
    active_context.gen = gen;
    active_context.caller = saved.fiber;
    fiber_switch(saved.fiber, &gen->fiber);
    active_context = saved;
    if (gen->done)
        return false;
    *v = gen->result;
    return true;
}

static void
gen_close(Generator *gen)
{
    if (!gen->done) {
        require(!gen->should_finish);
        gen->should_finish = true;
        Value v;
        gen_next(gen, &v);
        require(gen->done);
    }
    fiber_destroy(&gen->fiber);
    free(gen);
}

static void
gen_yield(const Value *v)
{
    Context cur = active_context;
    active_context.fiber = NULL;
    active_context.gen = NULL;
    active_context.caller = NULL;
    cur.gen->result = *v;
    fprintf(out, "[Generator[%d] %s] YIELD\n", cur.gen->id, cur.gen->name);
    fiber_switch(cur.fiber, cur.caller);
}

static bool
gen_should_finish()
{
    require(active_context.gen);
    return active_context.gen->should_finish;
}

typedef struct
{
    Generator *source;
    int n;
} TakeGenArgs;

static void
take_gen(void *args0)
{
    TakeGenArgs *args = (TakeGenArgs *) args0;
    while (args->n-- > 0 && !gen_should_finish()) {
        Value v;
        if (!gen_next(args->source, &v))
            break;
        gen_yield(&v);
    }
    gen_close(args->source);
}

static Generator *
gen_take(int n, Generator *source)
{
    TakeGenArgs args;
    args.source = source;
    args.n = n;
    return gen_new("gen_take", take_gen, &args, sizeof args);
}

static void
fib_gen(void *null)
{
    (void) null;
    uint64_t f0 = 0;
    uint64_t f1 = 1;
    Value v;
    v.u64 = f0;
    gen_yield(&v);
    while (!gen_should_finish()) {
        v.u64 = f1;
        gen_yield(&v);
        uint64_t tmp = f0;
        f0 = f1;
        f1 += tmp;
    }
}

static Generator *
gen_fibs()
{
    return gen_new("gen_fibs", fib_gen, NULL, 0);
}

typedef struct
{
    Generator *source;
    bool (*pred)(const Value *);
} FilterGenArgs;

static void
filter_gen(void *args0)
{
    FilterGenArgs *args = (FilterGenArgs *) args0;
    while (!gen_should_finish()) {
        Value v;
        if (!gen_next(args->source, &v))
            break;
        if (args->pred(&v))
            gen_yield(&v);
    }
    gen_close(args->source);
}

static Generator *
gen_filter(bool (*pred)(const Value *), Generator *source)
{
    FilterGenArgs args;
    args.source = source;
    args.pred = pred;
    return gen_new("gen_filter", filter_gen, &args, sizeof args);
}

static bool
is_odd_u64(const Value *v)
{
    return (v->u64 & 1) != 0;
}

int
main(int argc, char *argv[])
{
    test_main_begin(&argc, &argv);
    Fiber toplevel;
    gen_init_toplevel(&toplevel, &argc);
    Generator *gen = gen_take(20, gen_filter(is_odd_u64, gen_fibs()));
    Value v;
    while (gen_next(gen, &v))
        fprintf(out, "[Main] value: %" PRIu64 "\n", v.u64);
    gen_close(gen);
    test_main_end();
    return 0;
}
