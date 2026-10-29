# GDB helpers for the SRL profiler (saturnringlib/srl_profiler.hpp).
#
#   (gdb) source tools/scripts/srl_prof.gdb
#   (gdb) srl-prof-dump prof.bin
#
# Then on the PC: tools/scripts/srl_prof.py BuildDrop/<name>.elf prof.bin

define srl-prof-dump
  if $argc != 1
    echo usage: srl-prof-dump FILE\n
  else
    # Through the cache-through mirror (| 0x20000000): the slave writes part of the block,
    # and the master's cache may hold old copies of it.
    # The start address must have no spaces: GDB splits it from the end address on them.
    set $srl_prof_start = (char *)&g_srl_prof + 0x20000000
    dump binary memory $arg0 $srl_prof_start $srl_prof_start+sizeof(g_srl_prof)
    printf "wrote %d bytes, %d frames\n", sizeof(g_srl_prof), g_srl_prof.Head.Frames
  end
end

document srl-prof-dump
Writes the profiler's data block (g_srl_prof) to FILE, for tools/scripts/srl_prof.py.
The target must be stopped (Ctrl-C) first.
end

define srl-prof-dump-vram
  if $argc != 1
    echo usage: srl-prof-dump-vram FILE\n
  else
    # 2048 command slots of 32 bytes (SRL_PROFILER_MAX_VDP1_COMMANDS), for srl_prof.py --vram
    dump binary memory $arg0 0x25C00000 0x25C10000
    echo wrote the VDP1 command table\n
  end
end

document srl-prof-dump-vram
Writes the start of VDP1 VRAM (the command table) to FILE, so srl_prof.py --format vdp1 --vram FILE
can decode the commands of a GDB dump. The target must be stopped (Ctrl-C) first.
end
