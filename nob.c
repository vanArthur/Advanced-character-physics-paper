#define NOB_IMPLEMENTATION
#include "nob.h"

#define RAYLIB_VERSION "5.5"

// Platform detection and raylib URL/directory mapping
#if defined(_WIN32)
    #define PLATFORM_NAME "win64"
    #define RAYLIB_ARCHIVE "raylib-5.5_win64_msvc16.zip"
    #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_win64_msvc16.zip"
    #define RAYLIB_DIR "raylib-5.5_win64_msvc16"
    #define RAYLIB_LIBNAME "raylib.lib"
#elif defined(__APPLE__)
    #define PLATFORM_NAME "macos"
    #define RAYLIB_ARCHIVE "raylib-5.5_macos.tar.gz"
    #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_macos.tar.gz"
    #define RAYLIB_DIR "raylib-5.5_macos"
    #define RAYLIB_LIBNAME "libraylib.a"
#elif defined(__linux__)
    #define PLATFORM_NAME "linux_amd64"
    #define RAYLIB_ARCHIVE "raylib-5.5_linux_amd64.tar.gz"
    #define RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.tar.gz"
    #define RAYLIB_DIR "raylib-5.5_linux_amd64"
    #define RAYLIB_LIBNAME "libraylib.a"
#else
    #error "Unsupported platform"
#endif

bool download_and_extract_raylib(void) {
    nob_log(NOB_INFO, "Checking for raylib %s...", RAYLIB_VERSION);
    
    // Check if raylib directory already exists
    if (nob_file_exists(RAYLIB_DIR)) {
        nob_log(NOB_INFO, "raylib %s already exists at %s", RAYLIB_VERSION, RAYLIB_DIR);
        return true;
    }
    
    nob_log(NOB_INFO, "raylib not found. Downloading %s...", RAYLIB_ARCHIVE);
    
    // Download raylib
    Cmd cmd = {0};
    
#if defined(_WIN32)
    // Use PowerShell to download on Windows
    cmd_append(&cmd, "powershell", "-Command",
        nob_temp_sprintf("Invoke-WebRequest -Uri '%s' -OutFile '%s'", RAYLIB_URL, RAYLIB_ARCHIVE));
    if (!cmd_run(&cmd)) {
        nob_log(NOB_ERROR, "Failed to download raylib");
        return false;
    }
    
    // Extract using PowerShell
    cmd.count = 0;
    cmd_append(&cmd, "powershell", "-Command",
        nob_temp_sprintf("Expand-Archive -Path '%s' -DestinationPath '.' -Force", RAYLIB_ARCHIVE));
    if (!cmd_run(&cmd)) {
        nob_log(NOB_ERROR, "Failed to extract raylib");
        return false;
    }
#else
    // Use curl to download on Unix-like systems
    cmd_append(&cmd, "curl", "-L", "-o", RAYLIB_ARCHIVE, RAYLIB_URL);
    if (!cmd_run(&cmd)) {
        nob_log(NOB_ERROR, "Failed to download raylib");
        return false;
    }
    
    // Extract using tar
    cmd.count = 0;
    cmd_append(&cmd, "tar", "-xzf", RAYLIB_ARCHIVE);
    if (!cmd_run(&cmd)) {
        nob_log(NOB_ERROR, "Failed to extract raylib");
        return false;
    }
#endif
    
    nob_log(NOB_INFO, "Successfully downloaded and extracted raylib %s", RAYLIB_VERSION);
    return true;
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    
    // Download and extract raylib if needed
    if (!download_and_extract_raylib()) {
        return 1;
    }
    
    // Build the project
    Cmd cmd = {0};
    
#if defined(_WIN32)
    cmd_append(&cmd, "cl.exe");
    cmd_append(&cmd, "/nologo");
    cmd_append(&cmd, "/W3");
    cmd_append(&cmd, nob_temp_sprintf("/I%s\\include", RAYLIB_DIR));
    cmd_append(&cmd, "main.c");
    cmd_append(&cmd, nob_temp_sprintf("/link /LIBPATH:%s\\lib", RAYLIB_DIR));
    cmd_append(&cmd, RAYLIB_LIBNAME, "winmm.lib", "gdi32.lib", "user32.lib", "shell32.lib");
    cmd_append(&cmd, "/OUT:main.exe");
#else
    cmd_append(&cmd, "cc");
    cmd_append(&cmd, "-Wall", "-Wextra");
    cmd_append(&cmd, nob_temp_sprintf("-I./%s/include/", RAYLIB_DIR));
    cmd_append(&cmd, "-o", "main", "main.c");
    cmd_append(&cmd, nob_temp_sprintf("-L./%s/lib/", RAYLIB_DIR));
#if defined(__APPLE__)
    cmd_append(&cmd, "-lraylib");
    cmd_append(&cmd, "-framework", "CoreVideo");
    cmd_append(&cmd, "-framework", "IOKit");
    cmd_append(&cmd, "-framework", "Cocoa");
    cmd_append(&cmd, "-framework", "GLUT");
    cmd_append(&cmd, "-framework", "OpenGL");
#else
    cmd_append(&cmd, "-l:libraylib.a");
    cmd_append(&cmd, "-lm");
#endif
#endif
    
    if (!cmd_run(&cmd)) {
        return 1;
    }
    
    nob_log(NOB_INFO, "Build successful!");
    return 0;
}
