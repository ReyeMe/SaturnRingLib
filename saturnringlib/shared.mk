COMPILER_DIR=$(SDK_ROOT)/../Compiler

CMNDIR = $(COMPILER_DIR)/COMMON
CMODIR = $(COMPILER_DIR)/Other Utilities
IPFILE = $(CMNDIR)/IP.BIN
LDFILE = $(CMNDIR)/sgl.linker

BUILD_ELF = $(BUILD_DROP)/$(CD_NAME).elf
BUILD_ISO = $(BUILD_ELF:.elf=.iso)
BUILD_CUE = $(BUILD_ELF:.elf=.cue)
BUILD_RAW = $(BUILD_ELF:.elf=.raw)
BUILD_MAP = $(BUILD_ELF:.elf=.map)

STDDIR = $(COMPILER_DIR)/COMMON/other

SGLDIR = $(COMPILER_DIR)/COMMON/SGL_302j
SGLLDIR = $(SGLDIR)/LIB_ELF
SGLIDIR = $(SGLDIR)/inc
COBJECTS = $(SOURCES:.c=.o)
OBJECTS = $(COBJECTS:.cxx=.o)

# Handle defaults for user settings
ifeq (${SRL_CUSTOM_SGL_WORK_AREA},)
	SRL_CUSTOM_SGL_WORK_AREA = 0
endif

ifeq (${SRL_MAX_CD_BACKGROUND_JOBS},)
	SRL_MAX_CD_BACKGROUND_JOBS=1
endif

ifeq (${SRL_MAX_CD_FILES},)
	SRL_MAX_CD_FILES=255
endif

ifeq (${SRL_MAX_CD_RETRIES},)
	SRL_MAX_CD_RETRIES=5
endif

ifeq (${SRL_HIGH_RES}, 1)
	CCFLAGS += -DSRL_HIGH_RES
endif

ifeq (${SRL_FRAMERATE},)
	CCFLAGS += -DSRL_FRAMERATE=1
endif

ifeq (${SRL_MALLOC_MEMORY},)
	SRL_MALLOC_MEMORY=131072
endif

ifeq (${SRL_MAX_TEXTURES},)
	SRL_MAX_TEXTURES=100
endif

ifeq (${DEBUG}, 1)
	CCFLAGS += -DDEBUG
endif

ifneq (${SRL_MODE},PAL)
	ifneq (${SRL_MODE},NTSC)
		SRL_MODE = NTSC
	endif
endif

ifeq (${SRL_DEBUG_MAX_PRINT_LENGTH},)
	SRL_DEBUG_MAX_PRINT_LENGTH = 45
endif

CCFLAGS += -DSRL_MODE_${SRL_MODE} \
	-DSRL_MAX_TEXTURES=${SRL_MAX_TEXTURES} \
	-DSRL_MALLOC_MEMORY=${SRL_MALLOC_MEMORY} \
	-DSRL_MAX_CD_BACKGROUND_JOBS=${SRL_MAX_CD_BACKGROUND_JOBS} \
	-DSRL_MAX_CD_FILES=${SRL_MAX_CD_FILES} \
	-DSRL_MAX_CD_RETRIES=${SRL_MAX_CD_RETRIES} \
	-DSRL_DEBUG_MAX_PRINT_LENGTH=${SRL_DEBUG_MAX_PRINT_LENGTH}

# Handle work area
ifneq (${SRL_CUSTOM_SGL_WORK_AREA}, 1)
	SYSOBJECTS = $(SGLLDIR)/SGLAREA.O
endif

LIBS = $(SGLLDIR)/LIBCPK.A $(SGLLDIR)/LIBSND.A $(SGLLDIR)/SEGA_SYS.A $(SGLLDIR)/LIBCD.A $(SGLLDIR)/LIBSGL.A
CCFLAGS += -W -m2 -c -O2 -Wno-strict-aliasing -I$(SGLIDIR) -I$(STDDIR) -I$(SDK_ROOT)
LDFLAGS = -T$(LDFILE) -Wl,-Map,$(BUILD_MAP),-e,___Start -nostartfiles

# CD assets
ASSETS_DIR = ./cd/data
MUSIC_DIR = ./cd/music

%.o : %.c 
	sh2eb-elf-gcc.exe $< $(CCFLAGS) -std=c2x -o $@

%.o : %.cxx 
	sh2eb-elf-g++.exe $< $(CCFLAGS) -std=c++23 -fpermissive -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics -fno-use-cxa-atexit -o $@

compile_objects : $(OBJECTS)
	mkdir -p $(MUSIC_DIR)
	mkdir -p $(ASSETS_DIR)
	mkdir -p $(BUILD_DROP)
	sh2eb-elf-gcc.exe $(LDFLAGS) $(SYSOBJECTS) $(OBJECTS) $(LIBS) -o $(BUILD_ELF)
	
convert_binary : compile_objects
	sh2eb-elf-objcopy.exe -O binary $(BUILD_ELF) ./cd/data/0.bin

create_iso : convert_binary
	mkisofs -quiet -sysid "SEGA SATURN" -volid "SaturnApp" -volset "SaturnApp" -sectype 2352 \
	-publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "SaturnApp" \
	-abstract "$(ASSETS_DIR)/ABS.TXT" -copyright "$(ASSETS_DIR)/CPY.TXT" -biblio "$(ASSETS_DIR)/BIB.TXT" -generic-boot $(IPFILE) \
	-full-iso9660-filenames -o $(BUILD_ISO) $(ASSETS_DIR) $(ENTRYPOINT)

create_cue : create_iso
	cp -r $(MUSIC_DIR)/. $(BUILD_DROP)/
	cd $(BUILD_DROP); "$(CURDIR)/$(CMODIR)/JoEngineCueMaker.exe"; cd $(CURDIR)
	
clean:
	rm -f $(OBJECTS) $(BUILD_ELF) $(BUILD_ISO) $(BUILD_MAP) $(ASSETS_DIR)/0.bin
	rm -rf $(BUILD_DROP)/

build : create_cue
	
all: clean build
