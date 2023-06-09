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
	ODIR =
else
	ODIR = debug/
endif

DEFINES += -DENABLE_CHARSET_CONVERSION

CFLAGS  = -O2 -g -fomit-frame-pointer -fno-strict-aliasing \
          $(INCLUDES) $(DEFINES) $(WARNINGS)
LDFLAGS = -nostartfiles
LIBS    = 
STRIPFLAGS = -R.comment

ifneq (,$(findstring -aros,$(HOST)))
	DEFINES += -DNO_AROSC_LIB
	LIBS += -lstdc.static
endif

ifeq ($(HOST),m68k-amigaos)
	ARCH_000 = -mcpu=68000 -mtune=68000
	ARCH_020 = -mcpu=68020 -mtune=68020-60
	CFLAGS  := -noixemul -fno-common -mregparm $(CFLAGS)
	LDFLAGS := -noixemul $(LDFLAGS)
endif

main_SRCS = $(wildcard src/main/*.c)

SRCS = $(addprefix src/, \
       init.c filesysbox.c diskchange.c timer.c notify.c doslist.c lockhandler.c \
       fuse_stubs.c dopacket.c fsaddnotify.c fschangemode.c fsclose.c \
       fscreatedir.c fscreatehardlink.c fscreatesoftlink.c fscurrentvolume.c \
       fsdelete.c fsdie.c fsduplock.c fsexamineall.c fsexamineallend.c fsexaminelock.c \
       fsexaminenext.c fsformat.c fsinfodata.c fsinhibit.c fslock.c fsopen.c \
       fsopenfromlock.c fsparentdir.c fsread.c fsreadlink.c fsrelabel.c \
       fsremovenotify.c fsrename.c fssamelock.c fsseek.c fssetcomment.c \
       fssetdate.c fssetfilesize.c fssetownerinfo.c fssetprotection.c \
       fsunlock.c fswrite.c fswriteprotect.c volume.c xattrs.c utf8.c ucs4.c \
       strlcpy.c debugf.c dofmt.c allocvecpooled.c codesets.c avl.c)

OBJS = $(subst src/,$(ODIR)obj/,$(main_SRCS:.c=.o) $(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

ifeq ($(HOST),m68k-amigaos)
	OBJS_000 = $(subst obj/,obj-000/,$(OBJS))
	OBJS_020 = $(subst obj/,obj-020/,$(OBJS))
	DEPS_000 = $(OBJS_000:.o=.d)
	DEPS_020 = $(OBJS_020:.o=.d)
endif

.PHONY: all
ifeq ($(HOST),m68k-amigaos)
all: $(ODIR)$(TARGET).000 $(ODIR)$(TARGET).020
else
all: $(ODIR)$(TARGET) $(ODIR)$(TARGET).debug
endif

-include $(DEPS)

$(ODIR)obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(CFLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR)$(TARGET).debug: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ifneq (,$(findstring -aros,$(HOST)))
$(ODIR)$(TARGET): $(OBJS)
	$(CC) -s $(LDFLAGS) -o $@ $^ $(LIBS)
else
$(ODIR)$(TARGET): $(TARGET).debug
	$(STRIP) $(STRIPFLAGS) -o $@ $<
endif

ifeq ($(HOST),m68k-amigaos)
-include $(DEPS_000)
-include $(DEPS_020)

$(ODIR)obj-000/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_000) $(CFLAGS) $<
	$(CC) $(ARCH_000) $(CFLAGS) -c -o $@ $<

$(ODIR)obj-020/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_020) $(CFLAGS) $<
	$(CC) $(ARCH_020) $(CFLAGS) -c -o $@ $<

$(ODIR)$(TARGET).000: $(OBJS_000)
	$(CC) $(ARCH_000) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

$(ODIR)$(TARGET).020: $(OBJS_020)
	$(CC) $(ARCH_020) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug
endif

.PHONY: clean
clean:
	rm -rf $(TARGET) $(TARGET).debug obj
	rm -rf $(TARGET).000 $(TARGET).000.debug obj-000
	rm -rf $(TARGET).020 $(TARGET).020.debug obj-020
	rm -rf debug

.PHONY: revision
revision:
	bumprev -e si $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 src/filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc

