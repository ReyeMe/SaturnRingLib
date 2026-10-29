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
    dump binary memory $arg0 &g_srl_prof ((char *)&g_srl_prof) + sizeof(g_srl_prof)
    printf "wrote %d bytes, %d frames\n", sizeof(g_srl_prof), g_srl_prof.Head.Frames
  end
end

document srl-prof-dump
Writes the profiler's data block (g_srl_prof) to FILE, for tools/scripts/srl_prof.py.
The target must be stopped (Ctrl-C) first.
end
