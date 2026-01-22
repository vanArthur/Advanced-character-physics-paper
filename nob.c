#define NOB_IMPLEMENTATION
#include "nob.h"

Cmd command = {0};

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  cmd_append(&command, "cc");
  cmd_append(&command, "-Wall");
  cmd_append(&command, "-Wextra");
  cmd_append(&command, "-I./raylib-5.5_linux_amd64/include/");
  cmd_append(&command, "-o", "main", "main.c");
  cmd_append(&command, "-L./raylib-5.5_linux_amd64/lib/");
  cmd_append(&command, "-l:libraylib.a");
  cmd_append(&command, "-lm");

  if (!cmd_run(&command)) {
    return 1;
  }

  return 0;
}
