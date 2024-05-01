COMPILER_DIR=$(SDK_ROOT)/../Compiler

MODDIR = $(SDK_ROOT)/../modules
CMODIR = $(COMPILER_DIR)/Other Utilities

BUILD_ELF = $(BUILD_DROP)/$(CD_NAME).elf
BUILD_ISO = $(BUILD_ELF:.elf=.iso)
BUILD_CUE = $(BUILD_ELF:.elf=.cue)
BUILD_RAW = $(BUILD_ELF:.elf=.raw)
BUILD_MAP = $(BUILD_ELF:.elf=.map)

STDDIR = $(MODDIR)/danny/INC

SGLDIR = $(MODDIR)/sgl
IPFILE = $(SGLDIR)/IP.BIN
LDFILE = $(SGLDIR)/sgl.linker
SGLLDIR = $(SGLDIR)/LIB
SGLIDIR = $(SGLDIR)/INC

COBJECTS = $(SOURCES:.c=.o)
OBJECTS = $(COBJECTS:.cxx=.o)

# Handle defaults for user settings
ifeq ($(strip ${SRL_MAX_CD_BACKGROUND_JOBS}),)
	SRL_MAX_CD_BACKGROUND_JOBS=1
endif

ifeq ($(strip ${SRL_MAX_CD_FILES}),)
	SRL_MAX_CD_FILES=255
endif

ifeq ($(strip ${SRL_MAX_CD_RETRIES}),)
	SRL_MAX_CD_RETRIES=5
endif

ifeq ($(strip ${SRL_HIGH_RES}), 1)
	CCFLAGS += -DSRL_HIGH_RES
endif

ifeq ($(strip ${SRL_FRAMERATE}),)
	CCFLAGS += -DSRL_FRAMERATE=1
endif

ifeq ($(strip ${SRL_MALLOC_MEMORY}),)
	SRL_MALLOC_MEMORY=131072
endif

ifeq ($(strip ${SRL_MAX_TEXTURES}),)
	SRL_MAX_TEXTURES=100
endif

ifeq ($(strip ${DEBUG}), 1)
	CCFLAGS += -DDEBUG
endif

ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}), 1)
	CCFLAGS += -DSRL_USE_SGL_SOUND_DRIVER=$(strip ${SRL_USE_SGL_SOUND_DRIVER})
	
	ifeq ($(strip ${SRL_ENABLE_FREQ_ANALYSIS}),1)
		CCFLAGS += -DSRL_ENABLE_FREQ_ANALYSIS=1
	endif
endif

ifneq ($(strip ${SRL_MODE}),PAL)
	ifneq (${SRL_MODE},NTSC)
		SRL_MODE = NTSC
	endif
endif

ifeq ($(strip ${SRL_DEBUG_MAX_PRINT_LENGTH}),)
	SRL_DEBUG_MAX_PRINT_LENGTH = 45
endif

CCFLAGS += -DSRL_MODE_$(strip ${SRL_MODE}) \
	-DSRL_MAX_TEXTURES=$(strip ${SRL_MAX_TEXTURES}) \
	-DSRL_MALLOC_MEMORY=$(strip ${SRL_MALLOC_MEMORY}) \
	-DSRL_MAX_CD_BACKGROUND_JOBS=$(strip ${SRL_MAX_CD_BACKGROUND_JOBS}) \
	-DSRL_MAX_CD_FILES=$(strip ${SRL_MAX_CD_FILES}) \
	-DSRL_MAX_CD_RETRIES=$(strip ${SRL_MAX_CD_RETRIES}) \
	-DSRL_DEBUG_MAX_PRINT_LENGTH=$(strip ${SRL_DEBUG_MAX_PRINT_LENGTH}) \

# CD assets
ASSETS_DIR = ./cd/data
MUSIC_DIR = ./cd/music

# Handle work area
ifneq ($(strip ${SGL_MAX_VERTICES}),)
	SYSFLAGS += -DSGL_MAX_VERTICES=$(strip ${SGL_MAX_VERTICES})
else
	SYSFLAGS += -DSGL_MAX_VERTICES=2500
endif

ifneq ($(strip ${SGL_MAX_POLYGONS}),)
	SYSFLAGS += -DSGL_MAX_POLYGONS=$(strip ${SGL_MAX_POLYGONS})
else
	SYSFLAGS += -DSGL_MAX_POLYGONS=1700
endif

ifneq ($(strip ${SGL_MAX_EVENTS}),)
	SYSFLAGS += -DSGL_MAX_EVENTS=$(strip ${SGL_MAX_EVENTS})
else
	SYSFLAGS += -DSGL_MAX_EVENTS=64
endif

ifneq ($(strip ${SGL_MAX_WORKS}),)
	SYSFLAGS += -DSGL_MAX_WORKS=$(strip ${SGL_MAX_WORKS})
else
	SYSFLAGS += -DSGL_MAX_WORKS=256
endif

SYSSOURCES += $(SGLLDIR)/../SRC/workarea.c
SYSOBJECTS = $(SYSSOURCES:.c=.o)

# General compilation flags
LIBS = $(SGLLDIR)/LIBCPK.A $(SGLLDIR)/LIBSND.A $(SGLLDIR)/SEGA_SYS.A $(SGLLDIR)/LIBCD.A $(SGLLDIR)/LIBSGL.A
CCFLAGS += $(SYSFLAGS) -W -m2 -c -O2 -Wno-strict-aliasing -nostdlib -I$(SGLIDIR) -I$(STDDIR) -I$(SDK_ROOT)
LDFLAGS = -m2 -L$(SGLLDIR) -Xlinker -T$(LDFILE) -Xlinker -Map -Xlinker $(BUILD_MAP) -Xlinker -e -Xlinker ___Start -nostartfiles

# Compilation tasks
%.o : %.c 
	sh2eb-elf-gcc.exe $< $(CCFLAGS) -std=c2x -o $@

%.o : %.cxx 
	sh2eb-elf-g++.exe $< $(CCFLAGS) -std=c++23 -fpermissive -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics -fno-use-cxa-atexit -o $@

compile_objects : $(OBJECTS)
	mkdir -p $(MUSIC_DIR)
	mkdir -p $(ASSETS_DIR)
	mkdir -p $(BUILD_DROP)
	sh2eb-elf-gcc.exe $(LDFLAGS) $(SYSFLAGS) -nostdlib -m2 -c -g -O2 -I$(SGLIDIR) -o $(SYSOBJECTS) $(SYSSOURCES)
	sh2eb-elf-gcc.exe $(LDFLAGS) $(SYSOBJECTS) $(OBJECTS) $(LIBS) -o $(BUILD_ELF)
	
convert_binary : compile_objects
	sh2eb-elf-objcopy.exe -O binary $(BUILD_ELF) ./cd/data/0.bin

create_iso : convert_binary
ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}),1)
	cp -r $(SGLDIR)/DRV/. ./cd/data/
ifeq ($(strip ${SRL_ENABLE_FREQ_ANALYSIS}), 1)
	cp $(SGLDIR)/DSP/3BANDANA.EXB ./cd/data/
endif
endif
	mkisofs -quiet -sysid "SEGA SATURN" -volid "SaturnApp" -volset "SaturnApp" -sectype 2352 \
	-publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "SaturnApp" \
	-abstract "$(ASSETS_DIR)/ABS.TXT" -copyright "$(ASSETS_DIR)/CPY.TXT" -biblio "$(ASSETS_DIR)/BIB.TXT" -generic-boot $(IPFILE) \
	-full-iso9660-filenames -o $(BUILD_ISO) $(ASSETS_DIR) $(ENTRYPOINT)

create_cue : create_iso
	cp $(MUSIC_DIR)/*.wav $(BUILD_DROP)/ 2>/dev/null || :
	cd "$(BUILD_DROP)"; "$(CURDIR)/$(CMODIR)/JoEngineCueMaker.exe"; cd "$(CURDIR)"
	
clean:
	rm -f $(SGLLDIR)/../SRC/*.o
	rm -f $(OBJECTS) $(BUILD_ELF) $(BUILD_ISO) $(BUILD_MAP) $(ASSETS_DIR)/0.bin
ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}),1)
	rm -f $(ASSETS_DIR)/SDDRVS.DAT $(ASSETS_DIR)/SDDRVS.TSK $(ASSETS_DIR)/BOOTSND.MAP
ifeq ($(strip ${SRL_ENABLE_FREQ_ANALYSIS}), 1)
	rm -f $(ASSETS_DIR)/3BANDANA.EXB
endif
endif
	rm -rf $(BUILD_DROP)/

build : create_cue
	
all: clean build
