VERY_IMPORTANT_MESSAGE = "\"Nobody likes Flashback\""

pre_build:
	$(info ****** pre build step ******)
	echo "#define VERY_IMPORTANT_MESSAGE $(VERY_IMPORTANT_MESSAGE)" > ./tmp.h
