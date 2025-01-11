#include "raylib.h"
#include "platform_cdg.c"

int main() {

  InitWindow(640, 360, "sla");
  {
    while(!WindowShouldClose()){
      BeginDrawing();
      {

      }
      EndDrawing();
    }

  }
  CloseWindow();

  return 0;
}
