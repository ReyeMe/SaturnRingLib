COMPILER_DIR=$(SDK_ROOT)/../Compiler

MODDIR = $(SDK_ROOT)/../modules
CMODIR = $(COMPILER_DIR)/Other Utilities

STDDIR = $(MODDIR)/danny/INC

SGLDIR = $(MODDIR)/sgl
IPFILE = $(SGLDIR)/IP.BIN
LDFILE = $(SGLDIR)/sgl.linker
SGLLDIR = $(SGLDIR)/LIB
SGLIDIR = $(SGLDIR)/INC

LIBS = $(SGLLDIR)/LIBCPK.A $(SGLLDIR)/SEGA_SYS.A $(SGLLDIR)/LIBCD.A $(SGLLDIR)/LIBSGL.A

# include extra modules
MODULE_EXTRA_INC =

ifneq ($(strip $(MODULES_EXTRA)),)
	include $(patsubst %, $(SDK_ROOT)/../modules_extra/%/module.mk, $(strip $(MODULES_EXTRA)))
	MODULE_EXTRA_INC += $(patsubst %, -I$(SDK_ROOT)/../modules_extra/%/INC, $(strip $(MODULES_EXTRA)))
	MODULE_OBJECTS = $(MODULE_SOURCES:.c=.o)
	OBJECTS += $(MODULE_OBJECTS:.cxx=.o)
endif

COBJECTS = $(SOURCES:.c=.o)
OBJECTS += $(COBJECTS:.cxx=.o)

ifdef OS
   CC = sh2eb-elf-gcc.exe
   CXX = sh2eb-elf-g++.exe
   LD = sh2eb-elf-gcc.exe
   OBJCOPY = sh2eb-elf-objcopy.exe
else
	ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
		CC = sh2eb-elf-gcc
		CXX = sh2eb-elf-g++
		LD = sh2eb-elf-gcc
		OBJCOPY = sh2eb-elf-objcopy
	else
		# If YAUL is installed, use its compiler
		CC = ${YAUL_INSTALL_ROOT}/bin/sh2eb-elf-gcc
		CXX = ${YAUL_INSTALL_ROOT}/bin/sh2eb-elf-g++
		LD = ${YAUL_INSTALL_ROOT}/bin/sh2eb-elf-gcc
		OBJCOPY = ${YAUL_INSTALL_ROOT}/bin/sh2eb-elf-objcopy
  endif
endif

GCCMAJORVERSION = $(shell $(CC) --version | head -n 1 | sed 's/^.* //g' | sed 's/[^0-9].//g')

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
	CCFLAGS += -DSRL_FRAMERATE=0
else
	CCFLAGS += -DSRL_FRAMERATE=$(strip ${SRL_FRAMERATE})
endif

ifeq ($(strip ${SRL_MAX_TEXTURES}),)
	SRL_MAX_TEXTURES=100
endif

ifeq ($(strip ${DEBUG}), 1)
	CCFLAGS += -DDEBUG
endif

ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}), 1)
	CCFLAGS += -DSRL_USE_SGL_SOUND_DRIVER=$(strip ${SRL_USE_SGL_SOUND_DRIVER})
	LIBS += $(SGLLDIR)/LIBSND.A

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

ifeq ($(strip ${SRL_DEBUG_MAX_LOG_LENGTH}),)
	SRL_DEBUG_MAX_LOG_LENGTH = 80
endif

CCFLAGS += -DSRL_MODE_$(strip ${SRL_MODE}) \
	-DSRL_MAX_TEXTURES=$(strip ${SRL_MAX_TEXTURES}) \
	-DSRL_MAX_CD_BACKGROUND_JOBS=$(strip ${SRL_MAX_CD_BACKGROUND_JOBS}) \
	-DSRL_MAX_CD_FILES=$(strip ${SRL_MAX_CD_FILES}) \
	-DSRL_MAX_CD_RETRIES=$(strip ${SRL_MAX_CD_RETRIES}) \
	-DSRL_DEBUG_MAX_PRINT_LENGTH=$(strip ${SRL_DEBUG_MAX_PRINT_LENGTH}) \
	-DSRL_DEBUG_MAX_LOG_LENGTH=$(strip ${SRL_DEBUG_MAX_LOG_LENGTH}) \
	-DSRL_LOG_LEVEL=$(strip ${SRL_LOG_LEVEL}) \

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

BUILD_ELF = $(BUILD_DROP)/$(CD_NAME).elf
BUILD_ISO = $(BUILD_ELF:.elf=.iso)
BUILD_BIN = $(BUILD_ELF:.elf=.bin)
BUILD_CUE = $(BUILD_ELF:.elf=.cue)
BUILD_MAP = $(BUILD_ELF:.elf=.map)

TLSFDIR = $(MODDIR)/tlsf
DUMMYIDIR = $(MODDIR)/dummy

SYSSOURCES += $(SGLLDIR)/../SRC/workarea.c

ifdef SRL_MALLOC_METHOD
	ifeq ($(SRL_MALLOC_METHOD), TLSF)
		SYSSOURCES += $(TLSFDIR)/tlsf.c
		USE_TLSF_ALLOCATOR := TRUE
	endif
endif

SYSOBJECTS = $(SYSSOURCES:.c=.o)

# General compilation flags
CCFLAGS += $(SYSFLAGS) -W -m2 -c -O2 -Wno-strict-aliasing -nostdlib -ffreestanding -I$(DUMMYIDIR) -I$(SGLIDIR) -I$(STDDIR) -I$(TLSFDIR) -I$(SDK_ROOT) $(MODULE_EXTRA_INC)
LDFLAGS = -m2 -L$(SGLLDIR) -Xlinker -T$(LDFILE) -Xlinker -Map -Xlinker $(BUILD_MAP) -Xlinker -e -Xlinker ___Start -nostartfiles

ifeq "$(GCCMAJORVERSION)" "14"
    LDFLAGS += -specs=nosys.specs
endif

# Compilation tasks
%.o : %.c
	$(CC) $< $(CCFLAGS) -std=c2x -o $@

%.o : %.cxx
	$(CXX) $< $(CCFLAGS) -std=c++23 -fpermissive -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-threadsafe-statics -fno-use-cxa-atexit -o $@

compile_objects : $(OBJECTS) $(SYSOBJECTS)
	$(info ****** L@@K ******)
	$(info Maximum textures : ${SRL_MAX_TEXTURES})
	$(info Log level selected : ${SRL_LOG_LEVEL})
	$(info Maximum Log length : ${SRL_DEBUG_MAX_LOG_LENGTH})
	$(info ******************)
	mkdir -p $(MUSIC_DIR)
	mkdir -p $(ASSETS_DIR)
	mkdir -p $(BUILD_DROP)
	test -f $(ASSETS_DIR)/ABS.TXT || echo "NOT Abstracted by SEGA" >> $(ASSETS_DIR)/ABS.TXT
	test -f $(ASSETS_DIR)/BIB.TXT || echo "NOT Bibliographiced by SEGA" >> $(ASSETS_DIR)/BIB.TXT
	test -f $(ASSETS_DIR)/CPY.TXT || touch $(ASSETS_DIR)/CPY.TXT
	$(CC) $(LDFLAGS) $(SYSOBJECTS) $(OBJECTS) $(LIBS) -o $(BUILD_ELF)

convert_binary : compile_objects
	$(OBJCOPY) -O binary $(BUILD_ELF) ./cd/data/0.bin

create_iso : convert_binary
ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}),1)
	cp -r $(SGLDIR)/DRV/. ./cd/data/
ifeq ($(strip ${SRL_ENABLE_FREQ_ANALYSIS}), 1)
	cp $(SGLDIR)/DSP/3BANDANA.EXB ./cd/data/
endif
endif
	xorrisofs --norock -quiet -sysid "SEGA SATURN" -volid "SaturnApp" -volset "SaturnApp" \
	-publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "SaturnApp" \
	-abstract "$(ASSETS_DIR)/ABS.TXT" -copyright "$(ASSETS_DIR)/CPY.TXT" -biblio "$(ASSETS_DIR)/BIB.TXT" -generic-boot $(IPFILE) \
	-full-iso9660-filenames -o $(BUILD_ISO) $(ASSETS_DIR) $(ENTRYPOINT)

# Create CUE sheet
create_bin_cue: create_iso
	dd if=$(BUILD_ISO) of=$(BUILD_BIN) bs=2048
	echo 'FILE "$(CD_NAME).bin" BINARY' > $(BUILD_CUE)
	echo '  TRACK 01 MODE1/2048' >> $(BUILD_CUE)
	echo '    INDEX 01 00:00:00' >> $(BUILD_CUE)

AUDIO_FILES = $(patsubst ./%,%,$(shell find $(MUSIC_DIR) \( -name '*.mp3' -o -name '*.wav' -o -name '*.ogg' -o -name '*.flac' -o -name '*.aac' -o -name '*.m4a' -o -name '*.wma' \)))
AUDIO_FILES_RAW = $(patsubst %,%.raw,$(AUDIO_FILES))

%.raw: %
	sox $< -t raw -r 44100 -e signed-integer -b 16 -c 2 $@

add_audio_to_bin_cue: $(AUDIO_FILES_RAW)
	track=2; \
	total_size=$$(stat -c%s "$(BUILD_BIN)"); \
	for i in $^; do \
		sectors=$$((total_size / 2352)); \
		minutes=$$((sectors / (60 * 75))); \
		seconds=$$((sectors % (60 * 75) / 75)); \
		frames=$$((sectors % 75)); \
		echo '  TRACK' $$(printf "%02d" $$track) 'AUDIO' >> $(BUILD_CUE); \
		echo '    INDEX 01' $$(printf "%02d:%02d:%02d" $$minutes $$seconds $$frames) >> $(BUILD_CUE); \
		cat "$$i" >> $(BUILD_BIN); \
		track=$$((track + 1)); \
		size=$$(stat -c%s "$$i"); \
		total_size=$$((total_size + size)); \
	done

build_bin_cue: create_bin_cue add_audio_to_bin_cue

# CLONE_CD_PATH = $(BUILD_DROP)/CloneCdFiles
# CLONE_CD_CCD = $(CLONE_CD_PATH)/$(CD_NAME).ccd
# CLONE_CD_SUB = $(CLONE_CD_PATH)/$(CD_NAME).sub
# CLONE_CD_IMG = $(CLONE_CD_PATH)/$(CD_NAME).img

# create_clone_cd_files: build_bin_cue
# 	mkdir -p $(CLONE_CD_PATH)
# 	cp $(BUILD_BIN) $(CLONE_CD_IMG)
# 	touch $(CLONE_CD_SUB)
# 	echo "[CloneCD]" > $(CLONE_CD_CCD)
# 	echo "Version=3" >> $(CLONE_CD_CCD)
# 	echo "DiscType=CD" >> $(CLONE_CD_CCD)
# 	echo "Session=1" >> $(CLONE_CD_CCD)
# 	trackno=1; \
# 	while IFS= read -r line; do \
# 		if [[ $$line == TRACK* ]]; then \
# 			echo "[Session 1]" >> $(CLONE_CD_CCD); \
# 			echo "PreGapMode=2" >> $(CLONE_CD_CCD); \
# 			echo "PreGapSubC=0" >> $(CLONE_CD_CCD); \
# 			((trackno++)); \
# 		fi; \
# 		if [[ $$line == *BINARY ]]; then \
# 			echo "Point=0xA0" >> $(CLONE_CD_CCD); \
# 			echo "ADR=0x01" >> $(CLONE_CD_CCD); \
# 			echo "Control=0x04" >> $(CLONE_CD_CCD); \
# 			echo "TrackNo=$$trackno" >> $(CLONE_CD_CCD); \
# 			echo "AMin=0" >> $(CLONE_CD_CCD); \
# 			echo "ASec=0" >> $(CLONE_CD_CCD); \
# 			echo "AFrame=0" >> $(CLONE_CD_CCD); \
# 			echo "ALBA=-150" >> $(CLONE_CD_CCD); \
# 			echo "Zero=0" >> $(CLONE_CD_CCD); \
# 			echo "PMin=1" >> $(CLONE_CD_CCD); \
# 			echo "PSec=0" >> $(CLONE_CD_CCD); \
# 			echo "PFrame=0" >> $(CLONE_CD_CCD); \
# 			echo "PLBA=0" >> $(CLONE_CD_CCD); \
# 		fi; \
# 		if [[ $$line == *INDEX* ]]; then \
# 			IFS=':' read -ra ADDR <<< "$${line##* }"; \
# 			echo "Point=0x01" >> $(CLONE_CD_CCD); \
# 			echo "ADR=0x01" >> $(CLONE_CD_CCD); \
# 			echo "Control=0x04" >> $(CLONE_CD_CCD); \
# 			echo "TrackNo=$$trackno" >> $(CLONE_CD_CCD); \
# 			echo "AMin=$${ADDR[0]}" >> $(CLONE_CD_CCD); \
# 			echo "ASec=$${ADDR[1]}" >> $(CLONE_CD_CCD); \
# 			echo "AFrame=$${ADDR[2]}" >> $(CLONE_CD_CCD); \
# 			echo "ALBA=-150" >> $(CLONE_CD_CCD); \
# 			echo "Zero=0" >> $(CLONE_CD_CCD); \
# 			echo "PMin=$${ADDR[0]}" >> $(CLONE_CD_CCD); \
# 			echo "PSec=$${ADDR[1]}" >> $(CLONE_CD_CCD); \
# 			echo "PFrame=$${ADDR[2]}" >> $(CLONE_CD_CCD); \
# 			echo "PLBA=0" >> $(CLONE_CD_CCD); \
# 		fi; \
# 	done < $(BUILD_CUE)

clean:
	rm -f $(SGLLDIR)/../SRC/*.o
	rm -f $(OBJECTS) $(BUILD_ELF) $(BUILD_ISO) $(BUILD_MAP) $(ASSETS_DIR)/0.bin
	rm -f $(AUDIO_FILES_RAW)
ifeq ($(strip ${SRL_USE_SGL_SOUND_DRIVER}),1)
	rm -f $(ASSETS_DIR)/SDDRVS.DAT $(ASSETS_DIR)/SDDRVS.TSK $(ASSETS_DIR)/BOOTSND.MAP
ifeq ($(strip ${SRL_ENABLE_FREQ_ANALYSIS}), 1)
	rm -f $(ASSETS_DIR)/3BANDANA.EXB
endif
endif
	rm -rf $(BUILD_DROP)/

build : build_bin_cue

all: clean build
