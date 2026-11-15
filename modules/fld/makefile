# Rebuilds fld_knl.bin (the kernel SRL links by default) from src/fld_knl.c. Needs the sh-elf toolchain.
CC = sh-elf-gcc
OBJCOPY = sh-elf-objcopy
CFLAGS = -m2 -mb -Os -ffreestanding -nostdlib -fno-builtin -fno-strict-aliasing -Wall -Wextra -Werror

fld_knl.bin : fld_knl.elf
	$(OBJCOPY) -O binary $< $@

fld_knl.elf : src/fld_knl.c fld_knl.ld
	$(CC) $(CFLAGS) -T fld_knl.ld -Wl,--no-warn-rwx-segments -o $@ $<

clean :
	rm -f fld_knl.elf fld_knl.bin

.PHONY : clean
