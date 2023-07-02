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
	BINDIR = bin
	OBJDIR = obj
else
	BINDIR = bin/debug
	OBJDIR = obj/debug
endif

DEFINES += -DENABLE_CHARSET_CONVERSION

CFLAGS  = -O2 -g -fomit-frame-pointer -fno-strict-aliasing \
          $(INCLUDES) $(DEFINES) $(WARNINGS)
LDFLAGS = -nostartfiles
LIBS    = 
STRIPFLAGS = -R.comment

ifneq (,$(SYSROOT))
	CFLAGS  := --sysroot=$(SYSROOT) $(CFLAGS)
	LDFLAGS := --sysroot=$(SYSROOT) $(LDFLAGS)
endif

ifneq (,$(findstring -aros,$(HOST)))
	CPU = $(patsubst %-aros,%,$(HOST))
	DEFINES += -DNO_AROSC_LIB
	LIBS += -lstdc.static
endif

ifeq ($(HOST),m68k-amigaos)
	ARCH_000 = -mcpu=68000 -mtune=68000
	ARCH_020 = -mcpu=68020 -mtune=68020-60
	ARCH_060 = -mcpu=68060 -mtune=68060
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

ifeq ($(HOST),m68k-amigaos)
	OBJS_000 = $(subst src/,$(OBJDIR)/68000/,$(main_SRCS:.c=.o) $(SRCS:.c=.o))
	OBJS_020 = $(subst src/,$(OBJDIR)/68020/,$(main_SRCS:.c=.o) $(SRCS:.c=.o))
	OBJS_060 = $(subst src/,$(OBJDIR)/68060/,$(main_SRCS:.c=.o) $(SRCS:.c=.o))
	DEPS_000 = $(OBJS_000:.o=.d)
	DEPS_020 = $(OBJS_020:.o=.d)
	DEPS_060 = $(OBJS_060:.o=.d)
else
	OBJS = $(subst src/,$(OBJDIR)/$(CPU)/,$(main_SRCS:.c=.o) $(SRCS:.c=.o))
	DEPS = $(OBJS:.o=.d)
endif

.PHONY: all
ifeq ($(HOST),m68k-amigaos)
all: $(BINDIR)/$(TARGET).000 $(BINDIR)/$(TARGET).020 $(BINDIR)/$(TARGET).060
else
all: $(BINDIR)/$(TARGET).$(CPU) $(BINDIR)/$(TARGET).$(CPU).debug
endif

-include $(DEPS)

$(OBJDIR)/$(CPU)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(CFLAGS) $<
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/$(TARGET).$(CPU).debug: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ifneq (,$(findstring -aros,$(HOST)))
$(BINDIR)/$(TARGET).$(CPU): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) -s $(LDFLAGS) -o $@ $^ $(LIBS)
else
$(BINDIR)/$(TARGET).$(CPU): $(BINDIR)/$(TARGET).$(CPU).debug
	@mkdir -p $(dir $@)
	$(STRIP) $(STRIPFLAGS) -o $@ $<
endif

ifeq ($(HOST),m68k-amigaos)
-include $(DEPS_000)
-include $(DEPS_020)
-include $(DEPS_060)

$(OBJDIR)/68000/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_000) $(CFLAGS) $<
	$(CC) $(ARCH_000) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/68020/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_020) $(CFLAGS) $<
	$(CC) $(ARCH_020) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/68060/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -MM -MP -MT $(@:.o=.d) -MT $@ -MF $(@:.o=.d) $(ARCH_060) $(CFLAGS) $<
	$(CC) $(ARCH_060) $(CFLAGS) -c -o $@ $<

$(BINDIR)/$(TARGET).000: $(OBJS_000)
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_000) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

$(BINDIR)/$(TARGET).020: $(OBJS_020)
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_020) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug

$(BINDIR)/$(TARGET).060: $(OBJS_060)
	@mkdir -p $(dir $@)
	$(CC) $(ARCH_060) $(LDFLAGS) -o $@.debug $^ $(LIBS)
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug
endif

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: revision
revision:
	bumprev -e si $(VERSION) $(TARGET)

.PHONY: autodoc
autodoc:
	autodoc -l100 src/filesysbox_internal.h $(SRCS) $(main_SRCS) >filesysbox.doc

