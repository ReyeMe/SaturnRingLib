# Builds campaigns/fld_game and puts it on this disc as an FLD game:
#   GAME.BIN (its IP), GAME/0.BIN (first file of its directory), GAME/TEXT.TXT; TEXT.TXT in the root as well.
# BIG.BIN is an IP file one byte over the 8 KB ExecGame takes.
# GAME/0.BIN is padded to the largest size the loader takes (0xFC000 bytes: up to 0x06100000), over the menu's stack.
pre_build:
	$(info ****** Building fld_game ******)
	@$(MAKE) -C ../fld_game clean-preserve-audio convert_binary
	@mkdir -p ./cd/data/GAME
	@cp ../fld_game/cd/data/0.bin ./cd/data/GAME/0.BIN
	@truncate -s 1032192 ./cd/data/GAME/0.BIN
	@printf 'GAME directory' > ./cd/data/GAME/TEXT.TXT
	@printf 'root directory' > ./cd/data/TEXT.TXT
	@cp "$(IPFILE)" ./cd/data/GAME.BIN
	@cp "$(IPFILE)" ./cd/data/BIG.BIN
	@truncate -s 8193 ./cd/data/BIG.BIN
