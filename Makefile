
COMP_C ?= clang
COMP_CXX ?= clang++
COMP_ASM ?= yasm

CC := $(COMP_C)
CXX := $(COMP_CXX)
ASM := $(COMP_ASM)

#DEBUG_CLFAGS := 
DEBUG_CFLAGS ?= -ggdb -fstack-protector
DEBUG_ASMFLAGS ?= -g dwarf2 

CFLAGS := -Wall -Wextra -std=c99 $(DEBUG_CFLAGS) $(EXTRA_CFLAGS)
LDFLAGS := $(EXTRA_LDFLAGS)
ASMFLAGS := -f elf64 $(DEBUG_ASMFLAGS) $(EXTRA_ASMFLAGS)

all: test

test: test.o fiber.o fiber_asm.o

%.o: %.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

clean:
	- rm test *.o

.PHONY: clean
