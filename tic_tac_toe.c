#define DG_ARENA_DEBUG
#include "cdg_base.c"



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

#include <assert.h>
#include <stdio.h>
#include <math.h>

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
  Make_Dynamic_Array_type(struct mcts_node) dyn_children;
  Game_State state;
  Action action_taken;
  u64 visits;
  i64 wins;
} MCTS_Node;


MCTS_Node *uct_select(MCTS_Node *node, f32 exploration_constant, char player) {
  f32 best_score = -INFINITY;

  MCTS_Node *best_child = 0;
  Temorary_Arena_Memory tmp = temp_arena_memory_begin(node->backing_arena);
  {
    Make_Dynamic_Array_type(MCTS_Node*) best_children = {};
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

void apply_sigmoid(DG_Matrix m) {
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
        MAT_AT(m, r, c) = sigmoidf(MAT_AT(m, r, c));
    }
  }
}

void matrix_randomize(DG_Matrix m){
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
        MAT_AT(m, r, c) = randf()*(1.f - 0.f) + 0.f;
    }
  }
}

// xor {{{
typedef struct {
  DG_Matrix a0;

  DG_Matrix w1;
  DG_Matrix b1;
  DG_Matrix a1;

  DG_Matrix w2;
  DG_Matrix b2;
  DG_Matrix a2;
} Xor;

Xor xor_alloc(Arena *a){
  Xor model;
  model.a0 = matrix_alloc(a, 1, 2);
  model.a1 = matrix_alloc(a, 1, 2);
  model.a2 = matrix_alloc(a, 1, 1);

  model.w1 = matrix_alloc(a, 2, 2);
  model.w2 = matrix_alloc(a, 2, 1);

  model.b1 = matrix_alloc(a, 1, 2);
  model.b2 = matrix_alloc(a, 1, 1);
  return model;
}

void forward_xor(Xor model) {
  matrix_dot_in_place(model.a1, model.a0, model.w1);
  matrix_sum_in_place(model.a1, model.a1, model.b1);
  apply_sigmoid(model.a1);

  matrix_dot_in_place(model.a2, model.a1, model.w2);
  matrix_sum_in_place(model.a2, model.a2, model.b2);
  apply_sigmoid(model.a2);
}

f32 cost_xor(Xor model, DG_Matrix inputs, DG_Matrix outputs){
  DG_ASSERT(inputs.cols == model.a0.cols);
  DG_ASSERT(inputs.rows == outputs.rows);
  DG_ASSERT(outputs.cols == model.a2.cols);

  u32 number_of_samples = inputs.rows;

  f32 cost = 0;
  for (u32 i = 0; i < inputs.rows; ++i) {
    MAT_AT(model.a0, 0, 0) = MAT_AT(inputs, i, 0);
    MAT_AT(model.a0, 0, 1) = MAT_AT(inputs, i, 1);

    forward_xor(model);
    for (u32 j = 0; j < model.a2.cols; ++j) {
      f32 d = MAT_AT(model.a2, 0, j) - MAT_AT(outputs, i, j);
      cost += d * d;
    }
  }

  return cost / (f32)number_of_samples;
}

void finite_diff_xor(Xor model, Xor gradient, f32 epsilon, DG_Matrix inputs, DG_Matrix outputs) {
  f32 saved;
  f32 c = cost_xor(model, inputs, outputs);
  for (u32 i = 0; i < model.w1.rows; ++i){
    for (u32 j = 0; j < model.w1.cols; ++j){
      saved = MAT_AT(model.w1, i, j);
      MAT_AT(model.w1   , i, j) += epsilon;
      MAT_AT(gradient.w1, i, j) = (cost_xor(model, inputs, outputs) - c)/epsilon;
      MAT_AT(model.w1   , i, j) = saved;
    }
  }

  for (u32 i = 0; i < model.b1.rows; ++i){
    for (u32 j = 0; j < model.b1.cols; ++j){
      saved = MAT_AT(model.b1, i, j);
      MAT_AT(model.b1   , i, j) += epsilon;
      MAT_AT(gradient.b1, i, j) = (cost_xor(model, inputs, outputs) - c)/epsilon;
      MAT_AT(model.b1   , i, j) = saved;
    }
  }

  for (u32 i = 0; i < model.w2.rows; ++i){
    for (u32 j = 0; j < model.w2.cols; ++j){
      saved = MAT_AT(model.w2, i, j);
      MAT_AT(model.w2   , i, j) += epsilon;
      MAT_AT(gradient.w2, i, j) = (cost_xor(model, inputs, outputs) - c)/epsilon;
      MAT_AT(model.w2   , i, j) = saved;
    }
  }

  for (u32 i = 0; i < model.b2.rows; ++i){
    for (u32 j = 0; j < model.b2.cols; ++j){
      saved = MAT_AT(model.b2, i, j);
      MAT_AT(model.b2   , i, j) += epsilon;
      MAT_AT(gradient.b2, i, j) = (cost_xor(model, inputs, outputs) - c)/epsilon;
      MAT_AT(model.b2   , i, j) = saved;
    }
  }
}

void learn_xor(Xor model, Xor gradient, f32 rate) {
  for (u32 i = 0; i < model.w1.rows; ++i){
    for (u32 j = 0; j < model.w1.cols; ++j){
      MAT_AT(model.w1, i, j) -= rate * MAT_AT(gradient.w1, i, j);
    }
  }
  for (u32 i = 0; i < model.b1.rows; ++i){
    for (u32 j = 0; j < model.b1.cols; ++j){
      MAT_AT(model.b1, i, j) -= rate * MAT_AT(gradient.b1, i, j);
    }
  }
  for (u32 i = 0; i < model.w2.rows; ++i){
    for (u32 j = 0; j < model.w2.cols; ++j){
      MAT_AT(model.w2, i, j) -= rate * MAT_AT(gradient.w2, i, j);
    }
  }
  for (u32 i = 0; i < model.b2.rows; ++i){
    for (u32 j = 0; j < model.b2.cols; ++j){
      MAT_AT(model.b2, i, j) -= rate * MAT_AT(gradient.b2, i, j);
    }
  }
}
// }}}

// model  {{{
typedef Make_Slice_Type(DG_Matrix) DG_Matrix_Slice;
typedef struct {
  DG_Matrix_Slice a;
  DG_Matrix_Slice w;
  DG_Matrix_Slice b;
} ML_Model;

typedef Make_Slice_Type(u32) u32_Slice;

ML_Model make_model(Arena *a, u32_Slice layers){
  DG_ASSERT(layers.len > 0);

  ML_Model res = {};
  make_slice(a, &res.a, layers.len);
  make_slice(a, &res.w, layers.len - 1);
  make_slice(a, &res.b, layers.len - 1);


  SLICE_AT(res.a, 0) = matrix_alloc(a, 1, layers.data[0]);
  for (u32 layer_idx = 0; layer_idx < layers.len - 1; ++layer_idx) {
    u32 layer_inputs  = layers.data[layer_idx];
    u32 layer_outputs = layers.data[layer_idx + 1];

    res.w.data[layer_idx] = matrix_alloc(a, layer_inputs, layer_outputs);
    res.b.data[layer_idx] = matrix_alloc(a, 1, layer_outputs);
    res.a.data[layer_idx + 1] = matrix_alloc(a, 1, layer_outputs);
  }

  return res;
}

ML_Model alloc_model(Arena *a, u32 nodes_input, u32 nodes_middle, u32 nodes_output){

  ML_Model res = {};

  make_slice(a, &res.a, 3);
  make_slice(a, &res.w, 2);
  make_slice(a, &res.b, 2);

  res.a.data[0] = matrix_alloc(a, 1, nodes_input);
  res.a.data[1] = matrix_alloc(a, 1, nodes_middle);
  res.a.data[2] = matrix_alloc(a, 1, nodes_output);

  res.w.data[0] = matrix_alloc(a, res.a.data[0].cols, res.a.data[1].cols);
  res.w.data[1] = matrix_alloc(a, res.a.data[1].cols, res.a.data[2].cols);

  res.b.data[0] = matrix_alloc(a, res.a.data[1].rows, res.a.data[1].cols);
  res.b.data[1] = matrix_alloc(a, res.a.data[2].rows, res.a.data[2].cols);

  return res;
}

void forward_model(ML_Model model) {
  u32 layers = model.w.len;
  for (u32 layer = 0; layer < layers; ++layer) {
    DG_Matrix input_layer = model.a.data[layer];
    DG_Matrix output_layer = model.a.data[layer+1];

    DG_Matrix layer_bias  = model.b.data[layer];
    DG_Matrix layer_weight = model.w.data[layer];

    matrix_dot_in_place(output_layer, input_layer, layer_weight);
    matrix_sum_in_place(output_layer, output_layer, layer_bias);
    apply_sigmoid(output_layer);
  }
}

f32 cost_model(ML_Model model, DG_Matrix inputs, DG_Matrix outputs) {
  DG_ASSERT(inputs.cols == model.a.data[0].cols);
  DG_ASSERT(inputs.rows == outputs.rows);
  DG_ASSERT(outputs.cols == SLICE_AT(model.a, -1).cols);

  u32 number_of_samples = inputs.rows;

  f32 cost = 0;
  for (u32 i = 0; i < number_of_samples; ++i) {
    for (u32 j = 0; j < inputs.cols; ++j) {
      MAT_AT(model.a.data[0], 0, j) = MAT_AT(inputs, i, j);
    }

    forward_model(model);

    DG_Matrix last_layer = SLICE_AT(model.a, -1);
    for (u32 j = 0; j < last_layer.cols; ++j) {
      f32 d = MAT_AT(last_layer, 0, j) - MAT_AT(outputs, i, j);
      cost += d * d;
    }
  }

  return cost / (f32)number_of_samples;
}

void finite_diff_model(ML_Model model, ML_Model gradient, f32 epsilon, DG_Matrix inputs, DG_Matrix outputs) {
  f32 saved;
  f32 c = cost_model(model, inputs, outputs);

  u32 number_of_layers = model.w.len;
  for (u32 layer = 0; layer < number_of_layers; ++layer) {
    for (u32 stride_idx = 0; stride_idx  < 2; ++stride_idx ) {
      u32 stride = (u32[2]){ DG_OFFSET_OF(ML_Model, w), DG_OFFSET_OF(ML_Model, b) }[stride_idx];
      
      DG_Matrix model_layer    = ((DG_Matrix_Slice *)DG_DYNAMIC_ACCESS(&model   , stride))->data[layer];
      DG_Matrix gradient_layer = ((DG_Matrix_Slice *)DG_DYNAMIC_ACCESS(&gradient, stride))->data[layer];
      for (u32 i = 0; i < model_layer.rows; ++i) {
        for (u32 j = 0; j < model_layer.cols; ++j) {
          saved = MAT_AT(model_layer, i, j);
          MAT_AT(model_layer, i, j) += epsilon;
          MAT_AT(gradient_layer, i, j) = (cost_model(model, inputs, outputs) - c)/epsilon;
          MAT_AT(model_layer, i, j) = saved;
        }
      }
    }
  }
}

void learn_model(ML_Model model, ML_Model gradient, f32 rate) {

  u32 number_of_layers = model.w.len;
  for (u32 layer = 0; layer < number_of_layers; ++layer) {
    for (u32 stride_idx = 0; stride_idx  < 2; ++stride_idx ) {
      u32 stride = (u32[2]){ DG_OFFSET_OF(ML_Model, w), DG_OFFSET_OF(ML_Model, b) }[stride_idx];

      DG_Matrix model_layer    = ((DG_Matrix_Slice *)DG_DYNAMIC_ACCESS(&model   , stride))->data[layer];
      DG_Matrix gradient_layer = ((DG_Matrix_Slice *)DG_DYNAMIC_ACCESS(&gradient, stride))->data[layer];

      for (u32 i = 0; i < model_layer.rows; ++i){
        for (u32 j = 0; j < model_layer.cols; ++j){
          MAT_AT(model_layer, i, j) -= rate * MAT_AT(gradient_layer, i, j);
        }
      }
    }
  }

}

void randomize_model(ML_Model model) {
  DG_ASSERT(model.b.len == model.w.len);

  for (u32 i = 0; i < model.w.len; ++i) {
    matrix_randomize(model.w.data[i]);
    matrix_randomize(model.b.data[i]);
  }
}

ML_Model copy_model_structure(Arena *a, ML_Model model) {
  u32_Slice layers;
  make_slice(a, &layers, model.a.len);
  for (u32 i = 0; i < model.a.len; ++i) {
    layers.data[i] = model.a.data[i].cols;
  }

  return make_model(a, layers);
}

// }}}

void use_model(Arena *a){

  DG_Matrix inputs = {
    .rows = 4,
    .cols = 2,
    .data = (f32[]){
      0, 0,
      0, 1,
      1, 0,
      1, 1,
    }
  };

  DG_Matrix outputs = {
    .rows = 4,
    .cols = 1,
    .data = (f32[]){
      0,
      1,
      1,
      0,
    }
  };


  f32 epsilon = 1e-1;
  f32 learning_rate = 1e-1;
  u32 epochs = 100000;

  f32 x = 1;
  f32 y = 1;

  Xor model = xor_alloc(a);
  Make_Dynamic_Array_type(u32) sla = {};
  dynamic_array_push(&sla, 2, a);
  dynamic_array_push(&sla, 2, a);
  dynamic_array_push(&sla, 1, a);
  ML_Model new_xor = make_model(a, DG_REINTERPRET_CAST(u32_Slice, sla));
  ML_Model new_xor_gradient = copy_model_structure(a, new_xor);
  // {{{
  Xor gradient = xor_alloc(a);
  matrix_randomize(model.w1);
  matrix_randomize(model.b1);
  matrix_randomize(model.w2);
  matrix_randomize(model.b2);

  printf("cost = %f\n", cost_xor(model, inputs, outputs));
  for (u32 i = 0; i < epochs; ++i) {
    finite_diff_xor(model, gradient, epsilon, inputs, outputs);
    learn_xor(model, gradient, learning_rate);
  }
  printf("cost = %f\n", cost_xor(model, inputs, outputs));

  MAT_AT(model.a0, 0, 0) = x;
  MAT_AT(model.a0, 0, 1) = y;
  forward_xor(model);
  printf("value = %f\n", *model.a2.data);
  // }}}
  // {{{

  randomize_model(new_xor);

  printf("cost = %f\n", cost_model(new_xor, inputs, outputs));
  for (u32 i = 0; i < epochs; ++i) {
    finite_diff_model(new_xor, new_xor_gradient, epsilon, inputs, outputs);
    learn_model(new_xor, new_xor_gradient, learning_rate);
  }
  printf("cost = %f\n", cost_model(new_xor, inputs, outputs));


  MAT_AT(new_xor.a.data[0], 0, 0) = x;
  MAT_AT(new_xor.a.data[0], 0, 1) = y;
  forward_model(new_xor);
  printf("value = %f\n", *new_xor.a.data[2].data);
  // }}}


}


// }}}

// TODO: remover
#if defined(_MSC_VER)
#define _scanf scanf_s
#else
#define _scanf scanf
#endif


// human input {{{
Action receive_stdin_input(Game_State *s) {
  printf("%c, provide a number between 1 and 9: ", s->player);
  int pos;

  int res = _scanf("%d", &pos);
  fflush(stdin);

  if (!res) exit(1);

  return (Action){
    s->player,
    pos-1
  };
}
// }}}

