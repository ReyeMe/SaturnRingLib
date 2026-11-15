# Builds the two games of the menu from games/game (GAME_NUMBER 1 and 2) and lays them out on the disc:
#   GAMEn.BIN     IP of game n (the standard SRL IP)
#   GAMEn/0.BIN   game n program, first file of its directory
#   GAMEn/...     its sound driver and TEXT.TXT
# and one CD-DA track per game (cd/music/tracklist): game 1 owns disc track 2, game 2 disc track 3.

# SRL root as seen from games/game
FLD_GAME_SRL_ROOT = $(if $(filter /%,$(SRL_INSTALL_ROOT)),$(SRL_INSTALL_ROOT),../../$(SRL_INSTALL_ROOT))

pre_build:
	$(info ****** Building the FLD games ******)
	@for n in 1 2; do \
		$(MAKE) -C games/game SRL_INSTALL_ROOT=$(FLD_GAME_SRL_ROOT) SRL_CUSTOM_CCFLAGS=-DGAME_NUMBER=$$n clean-preserve-audio convert_binary || exit 1; \
		mkdir -p ./cd/data/GAME$$n; \
		cp games/game/cd/data/0.bin ./cd/data/GAME$$n/0.BIN; \
		cp $(SGLDIR)/DRV/. -r ./cd/data/GAME$$n/; \
		echo "Hello from the GAME$$n directory" > ./cd/data/GAME$$n/TEXT.TXT; \
		cp "$(IPFILE)" ./cd/data/GAME$$n.BIN; \
		test -f ./cd/music/game$$n.wav || sox -n -r 44100 -c 2 -b 16 ./cd/music/game$$n.wav synth 3 sine $$((220 * n)) vol 0.3; \
	done
	@printf 'game1.wav\ngame2.wav\n' > ./cd/music/tracklist
