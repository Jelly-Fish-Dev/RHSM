# gdb script loaded by `make gdb` after connecting to gdbserver on the device.
# Prints the save library once it has loaded at startup, then lets the app run.

set pagination off
set confirm off
set print pretty on

# on_library_loaded(const Library *lib) runs once after scan + database load.
# Prints every game and its save files.
break on_library_loaded
commands
  silent
  printf "\n[library] %d games, %d files  (db %s)\n", lib->game_count, lib->file_count, lib->db_path
  set $i = 0
  while $i < lib->game_count
    set $g = &lib->games[$i]
    printf "[%s] %s  -- %d sram, %d states\n", $g->system, $g->title, $g->sram_count, $g->state_count
    set $j = 0
    while $j < $g->file_count
      set $f = &$g->files[$j]
      printf "    %-5s slot %2d  %-18s %9lld bytes  %s\n", $f->kind, $f->slot, $f->core, $f->size, $f->filename
      set $j = $j + 1
    end
    set $i = $i + 1
  end
  printf "\n"
  continue
end

echo \n[rhsm] Connected. The save list prints once it loads. Ctrl+C to pause, `quit` to exit.\n\n
continue
