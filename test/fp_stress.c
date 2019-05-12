#include "fiber/fiber.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

double A[20];

#define CAT0(a, b) a##b
#define CAT(a, b) CAT0(a, b)

#define TMP(a) CAT(CAT(tmp_, a), __LINE__)

#define STR0(a) #a
#define STR(a) STR0(a)

#define DEF_A(i, j, k, l)                                                      \
    double CAT(a, i) = A[i] + args->id;                                        \
    double CAT(a, j) = A[j] + 3 * args->id;                                    \
    double CAT(a, k) = A[k] - 1 * args->id;                                    \
    double CAT(a, l) = A[l] - 3 * args->id

#define STORE(i, j, k, l)                                                      \
    A[i] = CAT(a, i);                                                          \
    A[j] = CAT(a, j);                                                          \
    A[k] = CAT(a, k);                                                          \
    A[l] = CAT(a, l)

#define CLAMP(x, y, l)                                                         \
    while (x * x + y * y > l) {                                                \
        double TMP(x) = x;                                                     \
        double TMP(y) = y;                                                     \
        x = (TMP(x) + TMP(y)) * (1. / 17);                                     \
        y = (TMP(x) - TMP(y)) * (1. / 18);                                     \
    }

#define MIX_(a, b, c, d)                                                       \
    do {                                                                       \
        a = b * c - 31 * d + 1;                                                \
        b = a * b - c * d * 2 + 1;                                             \
        c = d * d + a * 3 - 1;                                                 \
        d = a + b - c - 11 * d - 1;                                            \
        CLAMP(a, b, 24);                                                       \
        CLAMP(c, d, 27);                                                       \
        CLAMP(a, d, 29);                                                       \
        CLAMP(b, c, 31);                                                       \
    } while (0)

#define MIX(i, j, k, l) MIX_(CAT(a, i), CAT(a, j), CAT(a, k), CAT(a, l))

#define PRINT1(i) printf("a%02d=%+.13le ", i, A[i])

#define PRINT(i, j, k, l)                                                      \
    PRINT1(i);                                                                 \
    PRINT1(j);                                                                 \
    PRINT1(k);                                                                 \
    PRINT1(l);                                                                 \
    puts("")

#define MAP(F)                                                                 \
    do {                                                                       \
        F(0, 1, 2, 3);                                                         \
        F(4, 5, 6, 7);                                                         \
        F(8, 9, 10, 11);                                                       \
        F(12, 13, 14, 15);                                                     \
        F(16, 17, 18, 19);                                                     \
    } while (0)

#define STORE_ALL MAP(STORE)

typedef struct
{
    Fiber *self;
    Fiber *caller;
    int n;
    int id;
    bool done;
} Args;

static void
print(int id)
{
    printf("Fiber[%d]\n", id);
    MAP(PRINT);
    puts("");
}

static void
entry(void *args0)
{
    Args *args = (Args *) args0;
    DEF_A(0, 1, 2, 3);
    DEF_A(4, 5, 6, 7);
    DEF_A(8, 9, 10, 11);
    DEF_A(12, 13, 14, 15);
    DEF_A(16, 17, 18, 19);

    for (int i = 0; i < args->n; ++i) {
        for (int i = 0; i < 16; ++i) {
            if (i & 1) {
                MIX(0, 2, 4, 6);
                MIX(1, 3, 5, 7);
            } else {
                MIX(8, 10, 12, 14);
                MIX(9, 11, 13, 15);
            }
            if (a2 > 0 || a11 < 0) {
                MIX(0, 16, 3, 17);
                MIX(12, 18, 15, 19);
            }
        }
        fiber_switch(args->self, args->caller);
        if (i % 50 == 0) {
            STORE_ALL;
            print(args->id);
        }
    }

    STORE_ALL;
    print(args->id);
    args->done = true;
    fiber_switch(args->self, args->caller);
}

static void
guard(Fiber *fiber, void *null)
{
    (void) fiber;
    (void) null;
    abort();
}

static void
setup_fiber(Fiber *caller, Fiber *fiber, Args **args, int id)
{
    (void) fiber_alloc(fiber, 16 * 1024, guard, NULL, FIBER_FLAG_GUARD_LO);
    fiber_reserve_return(fiber, entry, (void **) args, sizeof *args);
    (*args)->self = fiber;
    (*args)->caller = caller;
    (*args)->n = 256;
    (*args)->id = id;
    (*args)->done = false;
}

int
main()
{
#if HU_OS_WINDOWS_P && defined(_TWO_DIGIT_EXPONENT)
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif
    Fiber toplevel;
    fiber_init_toplevel(&toplevel);
    Fiber fiber1;
    Args *args1;
    setup_fiber(&toplevel, &fiber1, &args1, 1);
    Fiber fiber2;
    Args *args2;
    setup_fiber(&toplevel, &fiber2, &args2, 2);

    while (!args1->done || !args2->done) {
        if (!args1->done)
            fiber_switch(&toplevel, &fiber1);
        if (!args2->done)
            fiber_switch(&toplevel, &fiber2);
    }
    fiber_destroy(&fiber1);
    fiber_destroy(&fiber2);
    return 0;
}
