
COMP_C ?= gcc
COMP_CXX ?= g++
COMP_ASM ?= yasm

CC := $(COMP_C)
CXX := $(COMP_CXX)
ASM := $(COMP_ASM)

# DEBUG_CFLAGS := 
# DEBUG_ASMFLAGS :=

DEBUG_CFLAGS ?= -ggdb -fstack-protector
DEBUG_ASMFLAGS ?= -g dwarf2 

OPT_CFLAGS := -O3 -march=native

CFLAGS := -Wall -Wextra -std=c99 -DFIBER_BITS64 \
	$(OPT_CFLAGS) \
	$(DEBUG_CFLAGS) $(EXTRA_CFLAGS)

LDFLAGS := $(OPT_CFLAGS) $(EXTRA_LDFLAGS)
ASMFLAGS := -f elf64 $(DEBUG_ASMFLAGS) $(EXTRA_ASMFLAGS)

all: test test2

test: test.o fiber.o fiber_asm.o

test2: test2.o fiber.o fiber_asm.o

fiber_asm.o: fiber_asm_linux_amd64.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

clean:
	- rm test test2 *.o

.PHONY: clean
