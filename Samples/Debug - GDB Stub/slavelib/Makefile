
# Standalone static library containing SlaveCounterTask (the code that runs
# on the Slave SH-2 in this sample), built separately from the rest of the
# sample and linked back in as a .a -- see ../pre.makefile (builds this
# before the sample compiles/links) and ../makefile's SRL_CUSTOM_LDFLAGS
# (links it back in with --whole-archive; see that file's comment for why
# --whole-archive is required here).
#
# The config below is intentionally a byte-for-byte copy of ../makefile's
# own config block. slave_counter_task.cxx pulls in <srl.hpp>, whose headers
# size several structs/arrays from these same macros (SRL_MAX_TEXTURES,
# SGL_MAX_*, SRL_MALLOC_METHOD, ...) -- if this library's copy of any of
# them drifted from the sample's, this .o and the sample's other .o files
# would disagree about layout for shared SRL types, an ABI mismatch inside
# one process that no compiler warning or GDB use would necessarily surface.
# Keep this block in sync with ../makefile by hand if either ever changes.
SRL_MAX_TEXTURES = 100
SRL_MODE = NTSC
SRL_HIGH_RES = 0
SRL_HIGH_RES_NON_INTERLACED = 0
SRL_FRAMERATE = 1
SRL_MAX_CD_BACKGROUND_JOBS = 1
SRL_MAX_CD_FILES = 256
SRL_MALLOC_METHOD = SIMPLE
SRL_LOG_LEVEL = NONE
SRL_LOG_OUTPUT = DEV_CART
DEBUG = 1
SRL_CUSTOM_CCFLAGS = -g -O0
SRL_USE_SGL_SOUND_DRIVER = 0
SGL_MAX_VERTICES = 2500
SGL_MAX_POLYGONS = 1500
SGL_MAX_EVENTS = 1
SGL_MAX_WORKS = 1

# Same repo, one directory deeper than the sample itself.
SRL_INSTALL_ROOT ?= ../../..
SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
SOURCES = slave_counter_task.cxx

# Reused only for its CC/CXX toolchain detection, CCFLAGS computation, and
# the %.o:%.cxx pattern rule -- these are exactly what makes this library's
# object code match the rest of the sample. shared.mk also unconditionally
# defines its own 'all'/'build'/'clean' targets (the full ISO build
# pipeline) -- NOT what we want here, so this file deliberately never
# defines a target literally named 'all' or 'clean' (that would just add
# our recipe/prerequisites onto shared.mk's instead of replacing them) and
# overrides .DEFAULT_GOAL below so a bare `make -C slavelib` builds only
# $(LIBOUT), not the ISO pipeline.
include $(SDK_ROOT)/shared.mk

LIBOUT = libslavetask.a
.DEFAULT_GOAL := $(LIBOUT)

# e.g. sh2eb-elf-gcc -> sh2eb-elf-ar (same toolchain prefix shared.mk already
# resolved for CC, so this can't drift from it independently).
AR = $(subst gcc,ar,$(CC))

.PHONY: clean-slavetask
$(LIBOUT): slave_counter_task.o
	$(AR) rcs $@ $^

clean-slavetask:
	rm -f slave_counter_task.o $(LIBOUT)
