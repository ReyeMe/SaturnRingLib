# Disc content the tests need besides the sound driver:
#   two CD-DA tracks for the Cdda tests: disc track 2 (3 s, 440 Hz) and track 3 (3 s, 660 Hz)
#   SKYBL.CPK, the movie of the "VDP1 - Cinepak" sample, for the CinepakPlayer tests
pre_build:
	@mkdir -p ./cd/music
	@test -f ./cd/music/track2.wav || sox -n -r 44100 -c 2 -b 16 ./cd/music/track2.wav synth 3 sine 440 vol 0.3
	@test -f ./cd/music/track3.wav || sox -n -r 44100 -c 2 -b 16 ./cd/music/track3.wav synth 3 sine 660 vol 0.3
	@printf 'track2.wav\ntrack3.wav\n' > ./cd/music/tracklist
	@cp "$(SRL_INSTALL_ROOT)/Samples/VDP1 - Cinepak/cd/data/SKYBL.CPK" ./cd/data/
