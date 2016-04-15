# Author: Yannik Sembritzki
# License: GPLv2

# dependency generation loosely based on http://make.mad-scientist.net/papers/advanced-auto-dependency-generation
CFLAGS=-I . --sysroot=$(SYSROOT)
SRCS = $(wildcard *.c)
OBJS=$(SRCS:.c=.o)
DEPS := $(SRCS:.c=.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td
POSTCOMPILE = @mv -f $*.Td $*.d
.PHONY: all clean build

all: libsepol.a

libsepol.a: $(OBJS)
	$(AR) rcs libsepol.a $(OBJS)

%.o: %.c %.d
	$(CC) -c $(DEPFLAGS) $(CFLAGS) $< -o $@
	$(POSTCOMPILE)

%.d: ;

clean:
	rm -f *.o *.d libsepol.a

-include $(DEPS)
