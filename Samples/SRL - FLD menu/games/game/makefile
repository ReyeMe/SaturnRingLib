# Game started by the FLD menu. pre.makefile of the menu builds it twice, with GAME_NUMBER 1 and 2.
SRL_MAX_TEXTURES = 1
SRL_MODE = NTSC
SRL_HIGH_RES = 0
SRL_FRAMERATE = 1
SRL_MAX_CD_BACKGROUND_JOBS = 1
SRL_MAX_CD_FILES = 64
SRL_MAX_CD_RETRIES = 5

# CD-DA volume is set through the sound driver; its files are copied into the game directory by pre.makefile
SRL_USE_SGL_SOUND_DRIVER = 1
SRL_ENABLE_FREQ_ANALYSIS = 0

SGL_MAX_VERTICES = 100
SGL_MAX_POLYGONS = 100
SGL_MAX_EVENTS = 1
SGL_MAX_WORKS = 1

CD_NAME = FLD_game
BUILD_DROP = ./BuildDrop
SRL_INSTALL_ROOT ?= ../../../..

SOURCES = $(patsubst ./%,%,$(shell find src/ -name '*.c'))
SOURCES += $(patsubst ./%,%,$(shell find src/ -name '*.cxx'))

SDK_ROOT = $(SRL_INSTALL_ROOT)/saturnringlib
include $(SDK_ROOT)/shared.mk
