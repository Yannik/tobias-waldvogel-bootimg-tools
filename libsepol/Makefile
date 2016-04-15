# Author: Yannik Sembritzki
# License: GPLv2

# dependency generation loosely based on http://make.mad-scientist.net/papers/advanced-auto-dependency-generation
CFLAGS=-I . --sysroot=$(SYSROOT)
SRCS = $(wildcard *.c)
OBJS=$(SRCS:.c=.o)
ARCH=x86
DEPS := $(SRCS:.c=.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td
POSTCOMPILE = @mv -f $*.Td $*.d
.PHONY: all clean build

all: libsepol_$(ARCH).a

libsepol_$(ARCH).a: $(OBJS)
	$(AR) rcs libsepol_$(ARCH).a $(OBJS)

%.o: %.c %.d
	$(CC) -c $(DEPFLAGS) $(CFLAGS) $< -o $@
	$(POSTCOMPILE)

%.d: ;

clean:
	rm -f *.o *.d libsepol_$(ARCH).a

-include $(DEPS)
