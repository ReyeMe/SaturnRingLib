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
    # Plain addresses: the stub reads all memory through the cache-through mirror itself, so the
    # slave's writes are seen, and they stay inside the readme's `mem` regions (GDB refuses the rest).
    # The start address must have no spaces: GDB splits it from the end address on them.
    set $srl_prof_start = (char *)&g_srl_prof
    dump binary memory $arg0 $srl_prof_start $srl_prof_start+sizeof(g_srl_prof)
    # The timeline and histograms live apart, in low work RAM: append them, the file is one block.
    set $srl_prof_start = (char *)&g_srl_prof_buffers
    append binary memory $arg0 $srl_prof_start $srl_prof_start+sizeof(g_srl_prof_buffers)
    printf "wrote %d bytes, %d frames\n", sizeof(g_srl_prof) + sizeof(g_srl_prof_buffers), g_srl_prof.Head.Frames
  end
end

document srl-prof-dump
Writes the profiler's data (g_srl_prof, then g_srl_prof_buffers) to FILE, for tools/scripts/srl_prof.py.
The target must be stopped (Ctrl-C) first.
end

define srl-prof-dump-vram
  if $argc != 1
    echo usage: srl-prof-dump-vram FILE\n
  else
    # 2048 command slots of 32 bytes (SRL_PROFILER_MAX_VDP1_COMMANDS), for srl_prof.py --vram
    # VDP1 VRAM is outside the readme's `mem` regions: let GDB read it, then restore the default.
    set mem inaccessible-by-default off
    dump binary memory $arg0 0x05C00000 0x05C10000
    set mem inaccessible-by-default on
    echo wrote the VDP1 command table\n
  end
end

document srl-prof-dump-vram
Writes the start of VDP1 VRAM (the command table) to FILE, so srl_prof.py --format vdp1 --vram FILE
can decode the commands of a GDB dump. The target must be stopped (Ctrl-C) first.
end
