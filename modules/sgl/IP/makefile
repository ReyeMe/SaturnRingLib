# macro
COMPILER = ../../../Compiler/sh2eb-elf/bin
TOOLS = ../../../Compiler/sh2eb-elf/sh2eb-elf/bin
CC = $(COMPILER)/sh2eb-elf-gcc
AS = $(COMPILER)/sh2eb-elf-as
LD = ld
RM = rm
CONV = $(COMPILER)/sh2eb-elf-objcopy

# directory
SGLDIR = ../
SGLIDR = $(SGLDIR)/INC
SGLLDR = $(SGLDIR)/LIB
STDDIR = $(SGLDIR)/../danny/INC

# source_program
SRCS = smpsys.c
ASRCS = sys_id.s

# area_code
# Only Japan
# OBJS_AREA = $(SGLLDR)/SYS_AREJ.O
# Only America
#OBJS_AREA = $(SGLLDR)/SYS_AREU.O
# Only Europe
#OBJS_AREA = $(SGLLDR)/SYS_AREE.O
# Only Taiwan
#OBJS_AREA = $(SGLLDR)/SYS_ARET.O
# World Wide
OBJS_AREA = $(SGLLDR)/SYS_AREJ.O $(SGLLDR)/SYS_ARET.O $(SGLLDR)/SYS_AREU.O $(SGLLDR)/SYS_AREE.O

# ip objects
OBJS = $(ASRCS:.s=.o) $(SGLLDR)/SYS_SEC.O $(OBJS_AREA) $(SGLLDR)/SYS_INIT.O $(SRCS:.c=.o)
CLEAR_OBJS = $(ASRCS:.s=.o) $(SRCS:.c=.o)

LIBS = 

# option
CCFLAGS = -O2 -m2 -c -g0 -DUSE_BACKUP_TYPES -I$(SGLIDR)
ASFLAGS =

# -m2 must be specified in LDFLAGS so the linker will search the SH2 lib dirs
# Specify path of libsgl.a by using "-L" option

LDFLAGS = -m2 -Xlinker -T$(LDFILE) -nostdlib -nolibc -nostdinc -nostartfiles -Xlinker -Map \
          -Xlinker $(MPFILE)
          
DFLAGS =

BUILDDROP = BuildDrop
TARGET   = $(BUILDDROP)/IP.ELF
TARGET1  = $(BUILDDROP)/IP.BIN
LDFILE	 = ip.linker
MPFILE   = $(TARGET1:.BIN=.MAP)
MAKEFILE = Makefile

all: clean $(TARGET) $(TARGET1)

# Use gcc to link so it will automagically find correct libs directory

$(TARGET) : $(OBJS) $(MAKEFILE) $(LDFILE)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $@

$(TARGET1) : $(OBJS) $(MAKEFILE) $(LDFILE)
	$(CONV) -O binary $(TARGET) $(TARGET1)

# suffix
.SUFFIXES: .asm

%.o: %.s
	$(AS) $< $(ASFLAGS) $(_ASFLAGS) -o $@

%.o: %.c
	$(CC) $< $(DFLAGS) $(CCFLAGS) -o $@

clean:
	rm -rf $(BUILDDROP)/*
	mkdir -p $(BUILDDROP)
	rm -f *.o
	