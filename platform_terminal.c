#include "tic_tac_toe.c"
#include <time.h>

void terminal_render(Game_State *s) {
  system("clear");
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      char item = s->board[j + i*3];
      printf("%c ", item == 0 ? '-' : item);
    }
    printf("\n");
  }
}

int main() {

  srand(time(0));
  size_t size = 20 * MEGABYTE;
  Arena arena = arena_init_buffer(malloc(size), size);

  use_model(&arena);
  return 0;


  Game_State game_state = {
    .player = 'O'
  };


  char winner;
  while (!game_state.game_over) {
    terminal_render(&game_state);
    Action action = {0};
    // printf("current player: %c\n", game_state.player);
    if (game_state.player == 'O') {
      // action = minimax(&game_state);
      action = monte_carlo_tree_search(arena, &game_state, 1000000, sqrt(2));
      // action = receive_stdin_input(&game_state);
    } else {
      // action = minimax(&game_state);
      action = monte_carlo_tree_search(arena, &game_state, 1000000, sqrt(2));
      // action = receive_stdin_input(&game_state);
    }
    simulate(&game_state, action);
    winner = terminated(&game_state);
    if (winner) game_state.game_over = true;
  }
  terminal_render(&game_state);
  if (winner == '-') {
    printf("its a tie!\n");
  } else {
    printf("winner is %c!\n", winner);
  }

  return 0;
}
