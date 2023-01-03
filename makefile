HOST ?= i386-aros

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
	ARCH_000 = -m68000
	ARCH_020 = -m68020
	CFLAGS  := -noixemul -fno-common -DNODEBUG $(CFLAGS)
	LDFLAGS := -noixemul $(LDFLAGS)
	LIBS    := $(LIBS)
endif

main_SRCS = $(wildcard src/main/*.c)
main_OBJS = $(subst src/,obj/,$(main_SRCS:.c=.o))

SRCS = $(addprefix src/, \
       init.c filesysbox.c uptime.c doslist.c utf8.c ucs4.c strlcpy.c \
       debugf.c kputstr.c snprintf.c allocvecpooled.c)

OBJS = $(main_OBJS) $(subst src/,obj/,$(SRCS:.c=.o))

ifeq ($(HOST),m68k-amigaos)
	OBJS_000 = $(subst obj/,obj-000/,$(OBJS))
	OBJS_020 = $(subst obj/,obj-020/,$(OBJS))
endif

.PHONY: all
ifeq ($(HOST),m68k-amigaos)
all: $(TARGET).000 $(TARGET).020
else
all: $(TARGET)
endif

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

obj-000/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_000) $(CFLAGS) -c -o $@ $<

obj-020/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_020) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

ifeq ($(HOST),m68k-amigaos)
$(TARGET).000: $(OBJS_000)
	$(CC) $(ARCH_000) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

$(TARGET).020: $(OBJS_020)
	$(CC) $(ARCH_020) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug
endif

init.o: $(TARGET)_rev.h src/filesysbox_vectors.c src/filesysbox_vectors.h

$(main_OBJS): src/filesysbox_vectors.h

$(OBJS): src/filesysbox_internal.h

.PHONY: clean
clean:
	rm -rf $(TARGET) $(TARGET).debug obj obj-000 obj-020

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 src/filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc
