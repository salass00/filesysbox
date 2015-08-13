CPU  := i386
OS   := aros
HOST := $(CPU)-$(OS)

CC := $(HOST)-gcc
RM := rm -f

TARGET  := filesysbox.library
VERSION := 53

CFLAGS  := -O2 -s -Wall -Werror -Wwrite-strings -Iinclude -D__NOLIBBASE__
LDFLAGS := -nostartfiles
LIBS    := -ldebug

ifeq ($(HOST),m68k-amigaos)
	CFLAGS  := -noixemul $(CFLAGS)
	LDFLAGS := -noixemul $(LDFLAGS)
	LIBS    := 
endif

main_SRCS := $(wildcard main/*.c)
main_OBJS := $(main_SRCS:.c=.o)
SRCS := init.c filesysbox.c uptime.c doslist.c \
	utf8.c ucs4.c strlcpy.c debugf.c kputstr.c \
	vsnprintf.c allocvecpooled.c
OBJS := $(main_SRCS:.c=.o) $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

init.o: $(TARGET)_rev.h filesysbox_vectors.c filesysbox_vectors.h
$(main_OBJS): filesysbox_vectors.h
$(OBJS): filesysbox_internal.h

.PHONY: clean
clean:
	$(RM) $(TARGET) *.o main/*.o

.PHONY: dist-clean
dist-clean:
	$(RM) *.o main/*.o

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

