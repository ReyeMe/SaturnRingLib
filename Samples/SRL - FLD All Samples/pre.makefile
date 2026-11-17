# Builds every other sample (each folder of Samples/ with a makefile, except those listed in blacklist.txt) and puts
# them all on this disc:
#   SAMPLES.BIN   IP file shared by all samples (the standard SRL IP)
#   Snn/          cd/data of sample nn: its 0.bin (the first file of the directory) and its files
#   cd/music      the CD-DA tracks of every sample, one after the other, in each sample's own order
# and writes src/samples.gen.hpp: name, directory and first CD-DA disc track of each sample, for the menu.
# shortcut: every sample boots with SRL's IP.BIN; a sample with its own SRL_IPBIN would need its own IP file.

FLD_ALL_SELF = $(notdir $(CURDIR))
FLD_ALL_BLACKLIST = ./blacklist.txt
FLD_ALL_HEADER = ./src/samples.gen.hpp

# Prints the converted (.raw) CD-DA tracks of sample directory $1, in the order its own build puts them on its disc
# (same rules as add_audio_to_bin_cue in shared.mk: the tracklist, or else the audio files find lists)
define FLD_ALL_TRACKS
sample_tracks() { \
	music="$$1/cd/music"; \
	if [ -f "$$music/tracklist" ]; then \
		( cat "$$music/tracklist"; echo ) | sed 's/^\s*//;s/\s*$$//;/^$$/d;/^#/d;s/:.*$$//' | \
		while IFS= read -r name; do [ -f "$$music/$$name.raw" ] && echo "$$music/$$name.raw"; done; \
	elif [ -d "$$music" ]; then \
		find "$$music" \( -name '*.mp3' -o -name '*.wav' -o -name '*.ogg' -o -name '*.flac' -o -name '*.aac' -o -name '*.m4a' -o -name '*.wma' \) | \
		while IFS= read -r file; do [ -f "$$file.raw" ] && echo "$$file.raw"; done; \
	fi; \
	return 0; \
}
endef

pre_build:
	$(info ****** Building all samples ******)
	@rm -rf ./cd/data/S[0-9]* ./cd/music
	@mkdir -p ./cd/data ./cd/music
	@cp "$(IPFILE)" ./cd/data/SAMPLES.BIN
	@$(FLD_ALL_TRACKS); \
	echo "// Written by pre.makefile: the samples on this disc" > $(FLD_ALL_HEADER); \
	echo "static const Sample Samples[] = {" >> $(FLD_ALL_HEADER); \
	: > ./cd/music/tracklist; \
	id=0; track=2; \
	for dir in ../*/; do \
		name=$$(basename "$$dir"); \
		[ -f "$$dir/makefile" ] || continue; \
		sed 's/\r$$//;s/[[:space:]]*$$//;/^[[:space:]]*#/d' $(FLD_ALL_BLACKLIST) | grep -Fxq -- "$$name" && continue; \
		: "This disc itself, even when renamed: building it from here would never end"; \
		[ "$$name" = "$(FLD_ALL_SELF)" ] && continue; \
		id=$$((id + 1)); sid=$$(printf 'S%02d' $$id); \
		echo "****** $$sid: $$name ******"; \
		$(MAKE) -C "$$dir" SRL_INSTALL_ROOT="$(SRL_INSTALL_ROOT)" all || exit 1; \
		mkdir -p ./cd/data/$$sid; \
		cp -r "$$dir/cd/data/." ./cd/data/$$sid/; \
		first=$$track; n=0; \
		sample_tracks "$$dir" > .sample_tracks; \
		while IFS= read -r raw; do \
			n=$$((n + 1)); out=$${sid}_$$n.cdda; \
			cp "$$raw" ./cd/music/$$out.raw; \
			: "Empty, older stand-in for the source: shared.mk then uses the converted .raw as it is"; \
			touch -t 197001010000 ./cd/music/$$out; \
			echo "$$out" >> ./cd/music/tracklist; \
			track=$$((track + 1)); \
		done < .sample_tracks; \
		rm -f .sample_tracks; \
		printf '    { "%s", "%s", %d },\n' "$$(echo "$$name" | sed 's/[\\"]/\\&/g')" "$$sid" $$first >> $(FLD_ALL_HEADER); \
	done; \
	echo "};" >> $(FLD_ALL_HEADER); \
	echo "****** $$id samples, $$((track - 2)) CD-DA tracks ******"
