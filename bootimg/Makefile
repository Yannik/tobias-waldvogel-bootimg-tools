# Author: Yannik Sembritzki
# License: GPLv2

# dependency generation loosely based on http://make.mad-scientist.net/papers/advanced-auto-dependency-generation
CFLAGS=-I ../libsepol -I ../include2 --sysroot=$(SYSROOT)
LIBS="../libsepol/libsepol_$(ARCH).a"
SRCS = $(wildcard *.c)
OBJS=$(SRCS:.c=.o)
DEPS=$(SRCS:.c=.d)
DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td
POSTCOMPILE = @mv -f $*.Td $*.d
ARCH=x86
.PHONY: all clean build

all: bootimg_$(ARCH)

bootimg_$(ARCH): $(OBJS)
	$(CC) -o bootimg_$(ARCH) $(OBJS) -static $(LIBS)

%.o: %.c %.d 
	$(CC) -c $(DEPFLAGS) $(CFLAGS) $< -o $@
	$(POSTCOMPILE)

%.d: ;

clean:
	rm -f *.o *.d bootimg_$(ARCH)

-include $(DEPS)
