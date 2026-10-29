# Configuration
SRL_MAX_TEXTURES = 100          # Number of VDP1 texture slots
SRL_MODE = NTSC                 # Valid options are PAL or NTSC
SRL_HIGH_RES = 0                # 480i mode
SRL_HIGH_RES_NON_INTERLACED = 0 # 240p mode
SRL_FRAMERATE = 1               # Framerate control (0=dynamic, 1=< 60/value)
SRL_MAX_CD_BACKGROUND_JOBS = 1  # Maximum number of files GFS can open at once
SRL_MAX_CD_FILES = 256          # Maximum number of files on a CD
SRL_MALLOC_METHOD = SIMPLE
SRL_LOG_LEVEL = NONE            # Maximum log level to display
SRL_LOG_OUTPUT = NONE           # Log output method (DEV_CART, EMULATOR, NONE)

# Profiler: instrument only the workload, so main.cxx's loop isn't profiled
SRL_PROFILER = 1
SRL_PROFILE_SOURCES = src/workload.cxx
# Frames between reports streamed over the DevCart; 0 = off (read with GDB).
# Leave it at 0 when you use GDB: report bytes would reach GDB through ftx -g.
SRL_PROFILER_REPORT_FRAMES = 0

# DEBUG builds the GDB stub in (to pull the profile); profile optimised code anyway
DEBUG = 1
OPT_FLAGS = -O2

# Sound driver specific configuration
SRL_USE_SGL_SOUND_DRIVER = 0    # Disabled: not needed
SRL_ENABLE_FREQ_ANALYSIS = 0    # Set to 1 if you want to enable frequency analysis for CD audio, this will load a DSP program into effect slot 1, SGL sound driver must be enabled

# SGL configuration
SGL_MAX_VERTICES = 2500         # Number of vertices that can be used
SGL_MAX_POLYGONS = 1500         # Number of polygons that can be used
SGL_MAX_EVENTS = 1              # Number of events that can be used
SGL_MAX_WORKS = 1               # Number of works that can be used

# Disk name
CD_NAME = Debug_Profiler

# Directory build will be placed into
BUILD_DROP = ./BuildDrop

# SRL installation directory
SRL_INSTALL_ROOT ?= ../..

# Find all .c and .cxx files
SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c'))
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

# Include shared makefile
SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
include $(SDK_ROOT)/shared.mk
