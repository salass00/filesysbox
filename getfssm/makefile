HOST ?= i386-aros

CC    = $(HOST)-gcc
STRIP = $(HOST)-strip

TARGET  = FbxGetFSSM
VERSION = 54

INCLUDES = -I.
DEFINES  = -D__NOLIBBASE__
WARNINGS = -Werror -Wall -Wwrite-strings -Wno-attributes

CFLAGS  = -O2 -g -fomit-frame-pointer $(INCLUDES) $(DEFINES) $(WARNINGS)
LDFLAGS = -nostartfiles

ifneq (,$(SYSROOT))
	CFLAGS  := --sysroot=$(SYSROOT) $(CFLAGS)
	LDFLAGS := --sysroot=$(SYSROOT) $(LDFLAGS)
endif

ifneq (,$(findstring -aros,$(HOST)))
	CPU = $(patsubst %-aros,%,$(HOST))
endif

ifeq ($(HOST),m68k-amigaos)
	ARCH = -mcpu=68000 -mtune=68020-60
	CFLAGS  := $(ARCH) -noixemul -fno-common $(CFLAGS)
	LDFLAGS := $(ARCH) -noixemul $(LDFLAGS)
endif

SRCS = getfssm.c

ifeq ($(HOST),m68k-amigaos)
	OBJS = $(addprefix obj/,$(SRCS:.c=.o))
else
	OBJS = $(addprefix obj/$(CPU)/,$(SRCS:.c=.o))
endif

.PHONY: all

ifeq ($(HOST),m68k-amigaos)
all: bin/$(TARGET)

obj/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

bin/$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@.debug $^
	$(STRIP) $(STRIPFLAGS) -o $@ $@.debug
else
all: bin/$(TARGET).$(CPU) bin/$(TARGET).$(CPU).debug

obj/$(CPU)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

bin/$(TARGET).$(CPU).debug: $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

bin/$(TARGET).$(CPU): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) -s $(LDFLAGS) -o $@ $^
endif

.PHONY: clean
clean:
	rm -rf bin obj

.PHONY: revision
revision:
	bumprev -e si $(VERSION) $(TARGET)

