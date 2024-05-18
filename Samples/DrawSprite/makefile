# Configuration
SRL_MAX_TEXTURES = 100			# Number of VDP1 texture slots
SRL_MODE = NTSC					# Valid options are PAL or NTSC
SRL_HIGH_RES = 0				# 480i mode
SRL_FRAMERATE = 1				# Framerate control (0=dynamic, 1=< 60/value)
SRL_MAX_CD_BACKGROUND_JOBS = 1	# Maximum number of files GFS can open at once
SRL_MAX_CD_FILES = 255			# Maximum number of files on a CD
SRL_MAX_CD_RETRIES = 5			# Number of times to retry on unsuccessful read
SRL_MALLOC_METHOD = TLSF		# Allocation method: TLSF or SIMPLE are supported.

# SGL specific configuration
SRL_CUSTOM_SGL_WORK_AREA = 0	# Set to 1 if you are using your own SGL work area
SRL_USE_SGL_SOUND_DRIVER = 0	# Set to 1 if you want to use SGL sound driver, this will copy necessary files into the CD folder
SRL_ENABLE_FREQ_ANALYSIS = 0	# Set to 1 if you want to enable frequency analysis for CD audio, this will load a DSP program into effect slot 1, SGL sound driver must be enabled

# Disk name
CD_NAME = DrawSprite

# Directory build will be placed into
BUILD_DROP = ./BuildDrop

# Find all .c and .cxx files
SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c')) 
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

# Include shared makefile
SDK_ROOT = ../../saturnringlib
include $(SDK_ROOT)/shared.mk