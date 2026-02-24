HOST ?= m68k-amigaos

INCDIR ?= /home/salass00/Development/NDK3.9/include/include_i

OBJCOPY = $(HOST)-objcopy

CFLAGS = -Os -Wall

.PHONY: all
all: allocdosobject.o

allocdosobject.o: allocdosobject.bin
	$(OBJCOPY) -I binary -O amiga $< $@

allocdosobject.bin: allocdosobject.s
	vasmm68k_mot -Fbin -quiet -I$(INCDIR) -o $@ $<

.PHONY: clean
clean:
	$(RM) -f allocdosobject.o allocdosobject.bin

