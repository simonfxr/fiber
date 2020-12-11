#ifndef TEST_PRE_H
#define TEST_PRE_H

#include <hu/macros.h>
#include <stdio.h>
#include <stdlib.h>

static FILE *out;

#define require(x)                                                             \
    if (!(x)) {                                                                \
        fprintf(stderr, "requirement not met: %s\n", HU_TOSTR(x));             \
        abort();                                                               \
    }

static void
println(const char *s)
{
    fprintf(out, "%s\n", s);
}

static void
test_main_begin(int *argc, char ***argv)
{
    require(*argc == 1 || *argc == 2);
    if (*argc == 1) {
        out = stdout;
        return;
    }
    out = fopen((*argv)[1], "w");
    require(out != NULL);
}

static void
test_main_end()
{
    if (out != stdout) {
        fclose(out);
    }
}

#endif
