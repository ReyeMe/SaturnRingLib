# Configuration
SRL_MAX_TEXTURES = 100          # Number of VDP1 texture slots
SRL_MODE = NTSC                 # Valid options are PAL or NTSC
SRL_HIGH_RES = 0                # 480i mode
SRL_HIGH_RES_NON_INTERLACED = 0 # 240p mode
SRL_FRAMERATE = 1               # Framerate control (0=dynamic, 1=< 60/value)
SRL_MAX_CD_BACKGROUND_JOBS = 1  # Maximum number of files GFS can open at once
SRL_MAX_CD_FILES = 256          # Maximum number of files on a CD
SRL_MALLOC_METHOD = SIMPLE
SRL_LOG_LEVEL = NONE          	# Maximum log level to display
SRL_LOG_OUTPUT = DEV_CART    	# Log output method (DEV_CART, EMULATOR, NONE)

# Enable debug symbol
DEBUG = 1
USER_FLAGS = -g -O0
# -Islavelib: slave_counter_task.hpp now lives in slavelib/ (its .cxx is
# built separately there into libslavetask.a, see slavelib/Makefile), so
# main.cxx's unchanged `#include "slave_counter_task.hpp"` needs this to
# still resolve.
SRL_CUSTOM_CCFLAGS = $(USER_FLAGS) -Islavelib

# Sound driver specific configuration
SRL_USE_SGL_SOUND_DRIVER = 0    # Disabled: GDB stub doesn't need audio
SRL_ENABLE_FREQ_ANALYSIS = 0    # Set to 1 if you want to enable frequency analysis for CD audio, this will load a DSP program into effect slot 1, SGL sound driver must be enabled

# SGL configuration
SGL_MAX_VERTICES = 2500         # Number of vertices that can be used
SGL_MAX_POLYGONS = 1500         # Number of polygons that can be used
SGL_MAX_EVENTS = 1             	# Number of events that can be used
SGL_MAX_WORKS = 1             	# Number of works that can be used 

# Disk name
CD_NAME = Debug_GDBStub

# Directory build will be placed into
BUILD_DROP = ./BuildDrop

# SRL installation directory
SRL_INSTALL_ROOT ?= ../..

# Link in slavelib/libslavetask.a (built by pre.makefile's pre_build hook,
# before this makefile's own compile/link step runs -- see slavelib/Makefile).
# --whole-archive is required, not optional: SRL_CUSTOM_LDFLAGS is placed
# ahead of this sample's own .o files on the final link command line (see
# shared.mk's compile_objects recipe), so at the point the linker reads this
# .a nothing has referenced SlaveCounterTask's symbols yet -- a plain static
# archive only pulls in members needed to satisfy *already-pending* undefined
# symbols, so without --whole-archive the linker would skip it entirely and
# main.o's later reference to SlaveCounterTask would fail with an undefined
# reference. --whole-archive forces every member in unconditionally,
# regardless of ordering; --no-whole-archive scopes that back down so it
# doesn't also apply to the SGL/system .A libraries linked in afterward.
SRL_CUSTOM_LDFLAGS = -Wl,--whole-archive ./slavelib/libslavetask.a -Wl,--no-whole-archive

# Find all .c and .cxx files
SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c')) 
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

# Include shared makefile
SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
include $(SDK_ROOT)/shared.mk
