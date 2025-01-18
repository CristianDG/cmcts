#include "raylib.h"
#include "tic_tac_toe.c"

void draw_player(char player) {}

void draw_board(Game_State *state) {
  f32 window_height = GetRenderHeight();
  f32 window_width = GetRenderWidth();
  f32 left = (window_width / 2) - (window_height / 2);
  f32 right = (window_width / 2) + (window_height / 2);

  f32 spacing = window_height / 3;
  for (i32 line = 1; line < 3; ++line) {
    f32 y = line * spacing;
    DrawLineV((Vector2){left, y}, (Vector2){right, y}, WHITE);
  }
  for (i32 line = 0; line < 4; ++line) {
    f32 x = left + line * spacing;
    DrawLineV((Vector2){x, 0}, (Vector2){x, window_height}, WHITE);
  }
}

int main() {

  srand(time(0));

  Game_State game_state = {};

  InitWindow(640, 360, "sla");
  {
    while(!WindowShouldClose()){
      BeginDrawing();
      {
        draw_board(&game_state);
      }
      EndDrawing();
    }

  }
  CloseWindow();

  return 0;
}
