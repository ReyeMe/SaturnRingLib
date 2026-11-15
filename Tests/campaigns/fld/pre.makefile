# Builds campaigns/fld_game and puts it on this disc as an FLD game:
#   GAME.BIN (its IP), GAME/0.BIN (first file of its directory), GAME/TEXT.TXT; TEXT.TXT in the root as well.
pre_build:
	$(info ****** Building fld_game ******)
	@$(MAKE) -C ../fld_game clean-preserve-audio convert_binary
	@mkdir -p ./cd/data/GAME
	@cp ../fld_game/cd/data/0.bin ./cd/data/GAME/0.BIN
	@printf 'GAME directory' > ./cd/data/GAME/TEXT.TXT
	@printf 'root directory' > ./cd/data/TEXT.TXT
	@cp "$(IPFILE)" ./cd/data/GAME.BIN
