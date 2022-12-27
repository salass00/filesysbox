CPU  := i386
OS   := aros
HOST := $(CPU)-$(OS)

CC := $(HOST)-gcc
STRIP = $(HOST)-strip
RM := rm -f

TARGET  := filesysbox.library
VERSION := 54

CFLAGS  := -O2 -fomit-frame-pointer -g -Wall -Wwrite-strings -Wno-attributes -Werror -fno-strict-aliasing -Iinclude -D__NOLIBBASE__
LDFLAGS := -nostartfiles
LIBS    := -ldebug
STRIPFLAGS = 

ifeq ($(HOST),m68k-amigaos)
	CFLAGS  := -noixemul -m68020 -fno-common -fomit-frame-pointer $(CFLAGS)
	LDFLAGS := -noixemul -m68020 $(LDFLAGS)
	LIBS    := $(LIBS)
endif

main_SRCS := $(wildcard main/*.c)
main_OBJS := $(main_SRCS:.c=.o)
SRCS := init.c filesysbox.c uptime.c doslist.c \
	utf8.c ucs4.c strlcpy.c debugf.c kputstr.c \
	snprintf.c allocvecpooled.c
OBJS := $(main_SRCS:.c=.o) $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

init.o: $(TARGET)_rev.h filesysbox_vectors.c filesysbox_vectors.h
$(main_OBJS): filesysbox_vectors.h
$(OBJS): filesysbox_internal.h

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TARGET).debug *.o main/*.o

.PHONY: dist-clean
dist-clean:
	$(RM) *.o main/*.o

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

