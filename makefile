HOST  ?= i386-aros
DEBUG ?= 0

CC    = $(HOST)-gcc
STRIP = $(HOST)-strip

TARGET  = filesysbox.library
VERSION = 54

INCLUDES = -I./include -I. -I./src
DEFINES  = -D__NOLIBBASE__
WARNINGS = -Werror -Wall -Wwrite-strings -Wno-attributes

ifneq (1,$(DEBUG))
	DEFINES += -DNODEBUG
endif

DEFINES += -DENABLE_CHARSET_CONVERSION

CFLAGS  = -O2 -g -fomit-frame-pointer -fno-strict-aliasing \
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
       fsdelete.c fsdie.c fsduplock.c fsexamineall.c fsexamineallend.c fsexaminelock.c \
       fsexaminenext.c fsformat.c fsinfodata.c fsinhibit.c fslock.c fsopen.c \
       fsopenfromlock.c fsparentdir.c fsread.c fsreadlink.c fsrelabel.c \
       fsremovenotify.c fsrename.c fssamelock.c fsseek.c fssetcomment.c \
       fssetdate.c fssetfilesize.c fssetownerinfo.c fssetprotection.c \
       fsunlock.c fswrite.c fswriteprotect.c volume.c xattrs.c utf8.c ucs4.c \
       strlcpy.c debugf.c kputstr.c snprintf.c allocvecpooled.c codesets.c \
       avl.c)

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

$(TARGET).debug: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ifneq (,$(findstring -aros,$(HOST)))
$(TARGET): $(OBJS)
	$(CC) -s $(LDFLAGS) -o $@ $^ $(LIBS)
else
$(TARGET): $(TARGET).debug
	$(STRIP) $(STRIPFLAGS) -o $@ $<
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
	rm -rf $(TARGET) $(TARGET).debug obj
	rm -rf $(TARGET).000 $(TARGET).000.debug obj-000
	rm -rf $(TARGET).020 $(TARGET).020.debug obj-020

.PHONY: revision
revision:
	bumprev -e si $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 src/filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc

