#define DG_ARENA_DEBUG
#include "platform_cdg.c"

#include <assert.h>
#include <time.h>
#include <stdio.h>
#include <math.h>

typedef struct {
  char board[9];
  char player;
  bool game_over; // TODO: remover
} Game_State;

typedef struct {
  char player;
  uint8_t position;
} Action;

typedef struct Action_List {
  Action actions[9];
  uint8_t size;
} Action_List;

// TODO: arena allocator

/* NOTE: traversing the board:
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      char item = s->board[j + i*3];
    }
  }
*/

char terminated(Game_State *s) {

  const uint8_t win_cons[][3] = {
    {0, 1, 2},
    {3, 4, 5},
    {6, 7, 8},

    {0, 3, 6},
    {1, 4, 7},
    {2, 5, 8},

    {0, 4, 8},
    {2, 4, 6},
  };

  for (int try_index = 0; try_index < 8; ++try_index) {
    const uint8_t *try = win_cons[try_index];
    char player_winning = s->board[try[0]];
    bool won = true;
    for (int cell_index = 0; cell_index < 3; ++cell_index) {
      won = won && (s->board[try[cell_index]] == player_winning);
      if (!won) break;
    }
    if (won) {
      return player_winning;
    }
  }


  bool board_full = true;
  for (int i = 0; i < 9; ++i) {
    board_full = board_full && s->board[i] != 0;
  }
  if (board_full) return '-';

  return 0;
}

// int8_t eval(Game_State *s) {
//
// }

Action_List possible_actions(Game_State *s){
  Action_List res = {};

  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      char item = s->board[j + i*3];
      if (item == 0) {
        res.actions[res.size] = (Action){s->player, j + i*3};
        res.size++;
      }
    }
  }

  return res;
}


void simulate(Game_State *s, Action a) {
  assert(a.player == 'O' || a.player == 'X');

  char *slot = &s->board[a.position];
  if (*slot == 'X' || *slot == 'O') {
    return;
  }

  *slot = a.player;
  s->player = a.player == 'X' ? 'O' : 'X';

  // TODO: checar se ganhou o jogo

}

// static int debug_iterations = 0;

// static int iterations;
void render_ascii(Game_State *s) {
  // printf("------\n");
  system("clear");
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      char item = s->board[j + i*3];
      printf("%c ", item == 0 ? '-' : item);
    }
    printf("\n");
  }
  // printf("iterations: %d\n", debug_iterations);
  // debug_iterations = 0;
}

#define MAXIMISING_PLAYER 'X'
#define MINIMISING_PLAYER 'O'

// MINIMAX {{{

#define MINIMAX_MAX_VALUE INT8_MAX
#define MINIMAX_MIN_VALUE INT8_MIN

typedef struct {
  Action action;
  int8_t value;
} Minimax_Action;

Minimax_Action maximax(Game_State s, Action a, int8_t alpha, int8_t beta);
Minimax_Action minimin(Game_State s, Action a, int8_t alpha, int8_t beta);

Action minimax(Game_State *s) {
  Minimax_Action res = {};
  if (s->player == MAXIMISING_PLAYER) {
    res = maximax(*s, (Action){}, MINIMAX_MIN_VALUE, MINIMAX_MAX_VALUE);
  } else {
    res = minimin(*s, (Action){}, MINIMAX_MIN_VALUE, MINIMAX_MAX_VALUE);
  }
  return res.action;
}


int8_t termination_value(char winner) {
  if (winner == MAXIMISING_PLAYER) return 1;
  if (winner == MINIMISING_PLAYER) return -1;
  return 0;
}


Minimax_Action maximax(Game_State s, Action a, int8_t alpha, int8_t beta){
  // debug_iterations++;
  if (a.player) {
    simulate(&s, a);
    char termination = terminated(&s);
    if (termination) {
      return (Minimax_Action){
        .action = a,
        .value = termination_value(termination)
      };
    }
  }

  Action_List action_list = possible_actions(&s);
  assert(action_list.size > 0);

  Minimax_Action res = {
    .value = MINIMAX_MIN_VALUE
  };

  for (int i = 0; i < action_list.size; ++i) {
    Action action = action_list.actions[i];
    Minimax_Action temp = minimin(s, action, alpha, beta);
    if (temp.value > res.value) {
      res = (Minimax_Action){
        .action = action,
        .value = temp.value,
      };
      alpha = MAX(alpha, res.value);
    }
    if (res.value >= beta) {
      return res;
    }
  }

  return res;
}

Minimax_Action minimin(Game_State s, Action a, int8_t alpha, int8_t beta){
  // debug_iterations++;
  if (a.player) {
    simulate(&s, a);
    char termination = terminated(&s);
    if (termination) {
      return (Minimax_Action){
        .action = a,
        .value = termination_value(termination)
      };
    }
  }

  Action_List action_list = possible_actions(&s);
  assert(action_list.size > 0);

  Minimax_Action res = {
    .value = MINIMAX_MAX_VALUE
  };

  for (int i = 0; i < action_list.size; ++i) {
    Action action = action_list.actions[i];
    Minimax_Action temp = maximax(s, action, alpha, beta);
    if (temp.value < res.value) {
      res = (Minimax_Action){
        .action = action,
        .value = temp.value,
      };
      beta = MIN(beta, res.value);
    }
    if (res.value <= alpha) {
      return res;
    }
  }

  return res;
}

// }}}

// MCTS {{{

typedef struct mcts_node {
  Arena *backing_arena;
  struct mcts_node *parent;

  // TODO: remover
  // struct mcts_node *first_child;

  // struct mcts_node *sibling;
  Dynamic_Array(struct mcts_node) dyn_children;
  Game_State state;
  Action action_taken;
  u64 visits;
  i64 wins;
} MCTS_Node;


// FIXME: para `MINIMISING_PLAYER` essa função está escolhendo errado
MCTS_Node *uct_select(MCTS_Node *node, f32 exploration_constant, char player) {
  f32 best_score = -INFINITY;

  MCTS_Node *best_child = 0;
  Temorary_Arena_Memory tmp = temp_arena_memory_begin(node->backing_arena);
  {
    Dynamic_Array(MCTS_Node*) best_children = {};
    for (i32 i = 0; i < node->dyn_children.len; i++) {
      MCTS_Node *child = &node->dyn_children.data[i];
      f32 score = 0;
      if (child->visits == 0) {
        score = INFINITY;
      } else {
        f32 exploitation = (f32)child->wins / (f32)child->visits;
        f32 exploration = exploration_constant * (f32)sqrt(log((f64)node->visits) / (f64)child->visits);
        score = exploration + exploitation;
      }

      if (score > best_score) {
        best_score = score;
        dynamic_array_clear(&best_children);
        dynamic_array_push(&best_children, child, tmp.arena);
      } else if (score == best_score) {
        dynamic_array_push(&best_children, child, tmp.arena);
      }
    }
    assert(best_children.len > 0);
    u32 idx = rand() % best_children.len;
    // TODO: random selection
    best_child = best_children.data[idx];
  }
  temp_arena_memory_end(tmp);
  return best_child;
}


void expand(MCTS_Node *node){

  Action_List action_list = possible_actions(&node->state);
  assert(action_list.size > 0);

  for (int i = 0; i < action_list.size; ++i) {
    Action action = action_list.actions[i];
    Game_State state_copy = node->state;
    simulate(&state_copy, action);

    MCTS_Node new_node = {
      .backing_arena = node->backing_arena,
      .action_taken = action,
      .state = state_copy,
      // add to tree
      .parent = node,
      // add to linked list
    };
    dynamic_array_push(&node->dyn_children, new_node, node->backing_arena);

  }
}

void back_propagate(MCTS_Node *node, i32 result) {
  while (node) {
    node->visits += 1;
    node->wins += result;
    node = node->parent;
  }
}

Action get_random_action(Action_List *list) {
  i32 idx = rand() % list->size;
  return list->actions[idx];
}

i32 random_simulate(Game_State *s, char player) {
  // FIXME: descobrir pq está bugado e se está bugado, já que eu não sei escrever comentários que prestam
  char winner = 0;
  while (winner == 0) {
    Action_List list = possible_actions(s);
    if (list.size) {
      Action action = get_random_action(&list);
      simulate(s, action);
    }
    winner = terminated(s);
  }
  i32 res = 0;
  if (winner == '-') {
    res = 0;
  } else if (winner == player) {
    res = 1;
  } else {
    res = -1;
  }
  return res;
}

MCTS_Node *get_random_node(MCTS_Node *node) {
  i32 idx = rand() % node->dyn_children.len;
  MCTS_Node *res = &node->dyn_children.data[idx];
  assert(res != 0);
  return res;
}

MCTS_Node *get_best_child(MCTS_Node *node) {
  MCTS_Node *best_node = &node->dyn_children.data[0];
  for (i32 i = 0; i < node->dyn_children.len; ++i) {
    MCTS_Node *child = &node->dyn_children.data[i];
    if (child->wins > best_node->wins) {
      best_node = child;
    }
  }
  return best_node;
}

Action monte_carlo_tree_search(Arena scratch, Game_State *s, i32 iterations, f32 exploration_constant) {
  assert(iterations > 0);
  char player = s->player;

  MCTS_Node *root = arena_alloc(&scratch, sizeof(MCTS_Node));
  root->backing_arena = &scratch;
  root->state = *s;

  for (i32 i = 0; i < iterations; ++i) {
    MCTS_Node *node = root;

    // seleção
    while (node->dyn_children.len > 0 && !terminated(&node->state)) {
      node = uct_select(node, exploration_constant, player);
    }

    // expansão
    // NOTE: trocar por `dynamic_array_len(node->dyn_children) > 0`
    while (node->dyn_children.len == 0 && !terminated(&node->state)) {
      expand(node);
    }

    // simulação
    if (node->dyn_children.len > 0) {
      node = get_random_node(node);
    }

    // retropropagação
    i32 result = random_simulate(&node->state, player);
    back_propagate(node, result);

  }

  MCTS_Node *best_child = get_best_child(root);
  assert(root->visits == iterations);
  assert(best_child->visits <= iterations);
  return best_child->action_taken;
}


// }}}

// softmax {{{

// typedef struct {
//   CDG_Matrix *activations;
//   CDG_Matrix *weights;
//   CDG_Matrix *biases;
// } Model;


f32 sigmoid(f32 x) {
  return 1.f / (1.f + exp(-x));
}

void apply_sigmoid(CDG_Matrix m) {
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
        M_AT(m, r, c) = sigmoid(M_AT(m, r, c));
    }
  }
}

f32 randf(){
  return (f32) rand() / (f32) RAND_MAX;
}

void matrix_randomize(CDG_Matrix m){
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
        M_AT(m, r, c) = randf()*(1.f - 0.f) + 0.f;
    }
  }
}

void use_model(Arena *a){
  CDG_Matrix a0 = matrix_alloc(a, 1, 2);

  CDG_Matrix w1 = matrix_alloc(a, 2, 2);
  CDG_Matrix b1 = matrix_alloc(a, 1, 2);
  CDG_Matrix a1 = matrix_alloc(a, 1, 2);

  CDG_Matrix w2 = matrix_alloc(a, 2, 1);
  CDG_Matrix b2 = matrix_alloc(a, 1, 1);
  CDG_Matrix a2 = matrix_alloc(a, 1, 1);

  // init {{{
  M_AT(a0, 0, 0) = 0;
  M_AT(a0, 0, 1) = 1;

  matrix_randomize(w1);
  matrix_randomize(b1);
  matrix_randomize(w2);
  matrix_randomize(b2);
  // }}}

  // forward {{{
  matrix_dot_in_place(a1, a0, w1);
  matrix_sum_in_place(a1, a1, b1);
  apply_sigmoid(a1);

  matrix_dot_in_place(a2, a1, w2);
  matrix_sum_in_place(a2, a2, b2);
  apply_sigmoid(a2);
  // }}}

  printf("%f\n", *a2.data);
}



// }}}

// human input {{{
Action receive_stdin_input(Game_State *s) {
  printf("%c, provide a number between 1 and 9: ", s->player);
  int pos;

  int res = scanf("%d", &pos);
  fflush(stdin);

  if (!res) exit(1);

  return (Action){
    s->player,
    pos-1
  };
}
// }}}

int tic_tac_toe_main() {
  srand(time(0));
  Arena arena = arena_init_malloc(20 * MEGABYTE);

  use_model(&arena);
  return 0;


  Game_State game_state = {
    .player = 'O'
  };


  char winner;
  while (!game_state.game_over) {
    render_ascii(&game_state);
    Action action = {};
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
  render_ascii(&game_state);
  if (winner == '-') {
    printf("its a tie!\n");
  } else {
    printf("winner is %c!\n", winner);
  }

  return 0;
}
