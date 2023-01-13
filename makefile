HOST ?= i386-aros

CC    = $(HOST)-gcc
STRIP = $(HOST)-strip

TARGET  = filesysbox.library
VERSION = 54

DEBUG    = -g
INCLUDES = -I./include -I. -I./src
DEFINES  = -DNODEBUG -D__NOLIBBASE__
WARNINGS = -Werror -Wall -Wwrite-strings -Wno-attributes

DEFINES += -DENABLE_CHARSET_CONVERSION

CFLAGS  = -O2 -fomit-frame-pointer -fno-strict-aliasing $(DEBUG) \
          $(INCLUDES) $(DEFINES) $(WARNINGS)
LDFLAGS = -nostartfiles
LIBS    = -ldebug
STRIPFLAGS = -R.comment

ifneq (,$(findstring -aros,$(HOST)))
	LIBS := -larosc $(LIBS)
endif

ifeq ($(HOST),m68k-amigaos)
	ARCH_000 = -m68000
	ARCH_020 = -m68020
	CFLAGS  := -noixemul -fno-common $(CFLAGS)
	LDFLAGS := -noixemul $(LDFLAGS)
endif

main_SRCS = $(wildcard src/main/*.c)
main_OBJS = $(subst src/,obj/,$(main_SRCS:.c=.o))

SRCS = $(addprefix src/, \
       init.c filesysbox.c diskchange.c timer.c notify.c doslist.c \
       fuse_stubs.c dopacket.c fsaddnotify.c fschangemode.c fsclose.c \
       fscreatedir.c fscreatehardlink.c fscreatesoftlink.c fscurrentvolume.c \
       fsdelete.c fsduplock.c fsexamineall.c fsexamineallend.c fsexaminelock.c \
       fsexaminenext.c fsformat.c fsinfodata.c fsinhibit.c fslock.c fsopen.c \
       fsopenfromlock.c fsparentdir.c fsread.c fsreadlink.c fsrelabel.c \
       fsremovenotify.c fsrename.c fssamelock.c fsseek.c fssetcomment.c \
       fssetdate.c fssetfilesize.c fssetownerinfo.c fssetprotection.c \
       fsunlock.c fswrite.c fswriteprotect.c volume.c xattrs.c utf8.c ucs4.c \
       strlcpy.c debugf.c kputstr.c snprintf.c allocvecpooled.c codesets.c)

OBJS = $(main_OBJS) $(subst src/,obj/,$(SRCS:.c=.o))

ifeq ($(HOST),m68k-amigaos)
	main_OBJS_000 = $(subst obj/,obj-000/,$(main_OBJS))
	main_OBJS_020 = $(subst obj/,obj-020/,$(main_OBJS))
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

ifneq (,$(findstring -aros,$(HOST)))
$(TARGET).debug: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

$(TARGET): $(OBJS)
	$(CC) -s $(LDFLAGS) -o $@ $^ $(LIBS)
else
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug
endif

init.o: $(TARGET)_rev.h src/filesysbox_vectors.c src/filesysbox_vectors.h

$(main_OBJS): src/filesysbox_vectors.h

$(OBJS): src/filesysbox_internal.h

ifeq ($(HOST),m68k-amigaos)
obj-000/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_000) $(CFLAGS) -c -o $@ $<

obj-020/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_020) $(CFLAGS) -c -o $@ $<

$(TARGET).000: $(OBJS_000)
	$(CC) $(ARCH_000) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

$(TARGET).020: $(OBJS_020)
	$(CC) $(ARCH_020) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

obj-000/init.o: $(TARGET)_rev.h src/filesysbox_vectors.c src/filesysbox_vectors.h
obj-020/init.o: $(TARGET)_rev.h src/filesysbox_vectors.c src/filesysbox_vectors.h

$(main_OBJS_000): src/filesysbox_vectors.h
$(main_OBJS_020): src/filesysbox_vectors.h

$(OBJS_000): src/filesysbox_internal.h
$(OBJS_020): src/filesysbox_internal.h
endif

.PHONY: clean
clean:
	rm -rf $(TARGET) $(TARGET).debug obj obj-000 obj-020

.PHONY: revision
revision:
	bumprev $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 src/filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc

