CROSS := i386-aros
CC    := $(CROSS)-gcc
RM    := rm -f

TARGET  := filesysbox.library
VERSION := 53

CFLAGS  := -O2 -s -Wall -Werror -Wwrite-strings -Iinclude -D__NOLIBBASE__
LDFLAGS := -nostartfiles
LIBS    := 

main_SRCS := $(wildcard main/*.c)
main_OBJS := $(main_SRCS:.c=.o)
SRCS := init.c filesysbox.c uptime.c doslist.c utf8.c ucs4.c debugf.c
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

