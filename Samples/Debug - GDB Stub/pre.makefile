# Pre-build hook (see saturnringlib/shared.mk: `build : pre_build build_bin_cue post_build`).
# Builds slavelib/libslavetask.a before the main sample's own sources are
# compiled/linked, so it exists in time for SRL_CUSTOM_LDFLAGS (see makefile)
# to link it into the final ELF.
pre_build:
	$(MAKE) -C slavelib
