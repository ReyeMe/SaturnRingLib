# Configuration
SRL_CUSTOM_SGL_WORK_AREA = 0	# Set to 1 if you are using your own SGL work area
SRL_MALLOC_MEMORY = 131072		# How much memory malloc can use (in bytes)
SRL_MAX_TEXTURES = 100			# Number of VDP1 texture slots
SRL_MODE = PAL					# Valid options are PAL or NTSC
SRL_HIGH_RES = 0				# 480i mode
SRL_FRAMERATE = 1				# Framerate control (0=dynamic, 1=< 60/value)
SRL_MAX_CD_BACKGROUND_JOBS = 1	# Maximum number of files GFS can open at once
SRL_MAX_CD_FILES = 255			# Maximum number of files on a CD
SRL_MAX_CD_RETRIES = 5			# Number of times to retry on unsuccessful read

# Compilation parameters
DEBUG = 1

# Disk name
CD_NAME = TestBuild

# Directory build will be placed into
BUILD_DROP = ./BuildDrop

# Find all .c and .cxx files
SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c')) 
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

# Include shared makefile
SDK_ROOT = ../../saturnringlib
include $(SDK_ROOT)/shared.mk