#MAKEFILES = $(shell find . -mindepth 2 -maxdepth 2 -type f -iname Makefile -exec awk 'END {print $0 "," FILENAME}' {} \; )

#SUBDIRS   = $(filter-out ./,$(dir $(MAKEFILES)))
#$(info $$MAKEFILES is [${MAKEFILES}])

all:
		$(shell find . -mindepth 2 -maxdepth 2 -type f -iname Makefile -print0 | xargs -0 awk 'END {print $0 "," FILENAME} )

		#make all -f "{}" -C {}
