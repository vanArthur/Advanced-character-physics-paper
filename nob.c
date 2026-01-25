#define NOB_IMPLEMENTATION
#include "nob.h"

Cmd command = {0};

typedef struct {
  const char *name;
  const char *url;
  const char *dir;
  const char *archive;
} RaylibPlatform;

RaylibPlatform get_raylib_platform(void) {
  RaylibPlatform platform = {0};

#ifdef _WIN32
  platform.name = "Windows";
  platform.url = "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_mingw-w64.zip";
  platform.dir = "raylib-5.5_win64_mingw-w64";
  platform.archive = "raylib-5.5_win64_mingw-w64.zip";
#elif __APPLE__
  platform.name = "macOS";
  platform.url = "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_macos.tar.gz";
  platform.dir = "raylib-5.5_macos";
  platform.archive = "raylib-5.5_macos.tar.gz";
#elif __linux__
  platform.name = "Linux";
  platform.url = "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz";
  platform.dir = "raylib-5.5_linux_amd64";
  platform.archive = "raylib-5.5_linux_amd64.tar.gz";
#else
  #error "Unsupported platform"
#endif

  return platform;
}

bool ensure_raylib(RaylibPlatform platform) {
  // Check if raylib directory exists
  if (file_exists(platform.dir)) {
    nob_log(NOB_INFO, "Raylib already present at %s", platform.dir);
    return true;
  }

  nob_log(NOB_INFO, "Raylib not found. Downloading for %s...", platform.name);

  // Download raylib
  Cmd download_cmd = {0};
  cmd_append(&download_cmd, "curl", "-L", "-o", platform.archive, platform.url);
  if (!cmd_run(&download_cmd)) {
    nob_log(NOB_ERROR, "Failed to download raylib");
    return false;
  }

  // Extract archive
  Cmd extract_cmd = {0};
#ifdef _WIN32
  // Use PowerShell for extraction on Windows
  cmd_append(&extract_cmd, "powershell", "-Command", 
             temp_sprintf("Expand-Archive -Path %s -DestinationPath . -Force", platform.archive));
#else
  // Use tar for Unix-like systems
  cmd_append(&extract_cmd, "tar", "-xzf", platform.archive);
#endif

  if (!cmd_run(&extract_cmd)) {
    nob_log(NOB_ERROR, "Failed to extract raylib archive");
    return false;
  }

  nob_log(NOB_INFO, "Raylib downloaded and extracted successfully");

  // Clean up archive
  if (file_exists(platform.archive)) {
#ifdef _WIN32
    cmd_append(&command, "del", platform.archive);
#else
    Cmd rm_cmd = {0};
    cmd_append(&rm_cmd, "rm", platform.archive);
    cmd_run(&rm_cmd);
#endif
  }

  return true;
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  RaylibPlatform platform = get_raylib_platform();
  
  if (!ensure_raylib(platform)) {
    nob_log(NOB_ERROR, "Failed to ensure raylib is available");
    return 1;
  }

  // Build main application
  cmd_append(&command, "cc");
  cmd_append(&command, "-Wall");
  cmd_append(&command, "-Wextra");
  cmd_append(&command, temp_sprintf("-I./%s/include/", platform.dir));
  cmd_append(&command, "-o", "main", "main.c");
  cmd_append(&command, temp_sprintf("-L./%s/lib/", platform.dir));
  
#ifdef _WIN32
  cmd_append(&command, "-l:libraylib.a");
  cmd_append(&command, "-lopengl32", "-lgdi32", "-lwinmm");
#elif __APPLE__
  cmd_append(&command, "-lraylib");
  cmd_append(&command, "-framework", "Cocoa");
  cmd_append(&command, "-framework", "OpenGL");
  cmd_append(&command, "-framework", "IOKit");
#else
  cmd_append(&command, "-l:libraylib.a");
  cmd_append(&command, "-lm");
#endif

  if (!cmd_run(&command)) {
    return 1;
  }

  return 0;
}
