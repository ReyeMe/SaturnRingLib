# Configuration
SRL_MAX_TEXTURES = 100          # Number of VDP1 texture slots
SRL_MODE = NTSC                 # Valid options are PAL or NTSC
SRL_HIGH_RES = 0                # 480i mode
SRL_FRAMERATE = 1               # Framerate control (0=dynamic, 1=< 60/value)
SRL_MAX_CD_BACKGROUND_JOBS = 1  # Maximum number of files GFS can open at once
SRL_MAX_CD_FILES = 256          # Maximum number of files on a CD
SRL_MAX_CD_RETRIES = 5          # Number of times to retry on unsuccessful read
SRL_MALLOC_METHOD = SIMPLE

# Sound driver specific configuration
SRL_USE_SGL_SOUND_DRIVER = 1    # Set to 1 if you want to use SGL sound driver, this will copy necessary files into the CD folder
SRL_ENABLE_FREQ_ANALYSIS = 1    # Set to 1 if you want to enable frequency analysis for CD audio, this will load a DSP program into effect slot 1, SGL sound driver must be enabled

# SGL configuration
SGL_MAX_VERTICES = 2500         # Number of vertices that can be used
SGL_MAX_POLYGONS = 1500         # Number of polygons that can be used
SGL_MAX_EVENTS = 1             	# Number of events that can be used
SGL_MAX_WORKS = 1             	# Number of works that can be used 

# Disk name
CD_NAME = Sound_CDDA

# Directory build will be placed into
BUILD_DROP = ./BuildDrop

# Custom SOX filters for audio conversion
# These filters must be exported for the conversion process to access them
# Usage: Add ":filtername" to a track in the tracklist file (e.g., "trackname.mp3:normalize")
export SRL_SOX_FILTERS_normalize = highpass 80 bass -4 120 compand 0.3,1 -70,-70,-60,-40,-20,-15 -3 -90 0.2 norm -3
export SRL_SOX_FILTERS_radio = highpass 80 compand 0.3,1 6:-70,-60,-20 -5 -90 0.2 norm -3

# SRL installation directory
SRL_INSTALL_ROOT ?= ../..

# Find all .c and .cxx files
SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c')) 
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

# Include shared makefile
SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
include $(SDK_ROOT)/shared.mk