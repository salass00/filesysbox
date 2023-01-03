HOST = i386-aros

CC    = $(HOST)-gcc
STRIP = $(HOST)-strip

TARGET  = filesysbox.library
VERSION = 54

CFLAGS  = -O2 -g -fomit-frame-pointer -fno-strict-aliasing -Wall -Wwrite-strings \
          -Wno-attributes -Werror -I./include -I. -D__NOLIBBASE__
LDFLAGS = -nostartfiles
LIBS    = -ldebug
STRIPFLAGS = 

ifeq ($(HOST),m68k-amigaos)
	CFLAGS  := -noixemul -m68020 -fno-common -DNODEBUG $(CFLAGS)
	LDFLAGS := -noixemul -m68020 $(LDFLAGS)
	LIBS    := $(LIBS)
endif

main_SRCS = $(wildcard src/main/*.c)
main_OBJS = $(subst src/,obj/,$(main_SRCS:.c=.o))

SRCS = $(addprefix src/, \
       init.c filesysbox.c uptime.c doslist.c utf8.c ucs4.c strlcpy.c \
       debugf.c kputstr.c snprintf.c allocvecpooled.c)

OBJS = $(main_OBJS) $(subst src/,obj/,$(SRCS:.c=.o))

.PHONY: all
all: $(TARGET)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

init.o: $(TARGET)_rev.h src/filesysbox_vectors.c src/filesysbox_vectors.h

$(main_OBJS): src/filesysbox_vectors.h

$(OBJS): src/filesysbox_internal.h

.PHONY: clean
clean:
	rm -rf $(TARGET) $(TARGET).debug obj

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc

