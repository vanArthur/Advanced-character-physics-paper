#include <raylib.h>

#define WIDTH 1000
#define HEIGHT 1000


int main(void) {
  InitWindow(WIDTH, HEIGHT, "Advanced Character Physics");
  SetTargetFPS(60);

  int w = GetScreenWidth();
  int h = GetScreenHeight();

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(GetColor(0x052A4Fff));
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
