// types.h {{{
#ifndef CDG_TYPES_H
#define CDG_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* TODO:
 * - context cracking
 * - assert + debugger trap
 */


#ifndef DG_ASSERT
// TODO: trocar pelo meu assert
#include <assert.h>
#define DG_ASSERT assert
#define DG_ASSERT_MSG(check, msg) assert((check) && msg)
#endif // DG_ASSERT

#define DG_STATEMENT(x) do { x } while (0)

#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define CLAMP_TOP MIN
#define CLAMP_BOTTOM MAX

// TODO: olhar se funciona
#define STR(x) #x
#define GLUE(a,b) a##b

#define is_power_of_two(x) ((x != 0) && ((x & (x - 1)) == 0))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;
typedef intptr_t intptr;

typedef size_t usize;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

typedef u32 b32;

#define KILOBYTE 1024
#define MEGABYTE 1048576

#define DG_REINTERPRET_CAST(type, val) \
  (*((type *)&(val)))

#define DG_OFFSET_OF(type, field) \
  ((uintptr)&(((type *) 0)->field))

#define DG_DYNAMIC_ACCESS(type, offset) \
  (((void *)(type))+offset)

#endif
// }}}

// alloc.c {{{
#ifndef CDG_ALLOC_C
#define CDG_ALLOC_C

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for memset

#define DEFAULT_ALIGNMENT 16

// TODO: WIP
typedef struct {
} Growing_Arena;

typedef struct {
  u8 *data;
  // TODO: add temp_count
  u32 size;
  u32 cursor;
} Arena;

// TODO: adicionar uma abstração que faça uso
typedef struct {
  void *(*alloc)(u64);
  void (*free)(void *);
} DG_Allocator;

typedef struct {
  Arena *arena;
  u32 cursor;
} Temorary_Arena_Memory;

Arena arena_init_malloc(u64 size) {
  void *data = malloc(size);

  return (Arena){
    .data = data,
    .size = size,
  };
}

// implementation from https://dylanfalconer.com/articles/the-arena-custom-memory-allocators
uintptr_t align_forward(uintptr_t ptr, size_t alignment) {
    uintptr_t p, a, modulo;
    if (!is_power_of_two(alignment)) {
        return 0;
    }

    p = ptr;
    a = (uintptr_t)alignment;
    modulo = p & (a - 1);

    if (modulo) {
        p += a - modulo;
    }

    return p;
}

void *_arena_alloc(Arena *arena, size_t size, size_t alignment) {
  uintptr_t curr_ptr = (uintptr_t)arena->data + (uintptr_t)arena->cursor;
  uintptr_t offset = align_forward(curr_ptr, alignment);
  offset -= (uintptr_t)arena->data;

  // acredito que um if é melhor que um assert aqui
  if (offset + size > arena->size) {
    return 0;
  }

  void *ptr = (void *)arena->data + offset;
  arena->cursor = offset + size;
  return memset(ptr, 0, size);
}

// TODO: olhar https://youtu.be/443UNeGrFoM?si=DBJXmKB_z8W8Yrrf&t=3074
void *_tracking_arena_alloc(Arena *arena, size_t size, size_t alignment, char *file, i32 line) {
  // TODO: registrar onde foram todas as alocações
  void *ptr = _arena_alloc(arena, size, alignment);
  if (ptr == 0) {
    fprintf(stderr, "%s:%d Could not allocate %zu bytes\n", file, line, size);
  }
  return ptr;
}

Temorary_Arena_Memory temp_arena_memory_begin(Arena *a){
  return (Temorary_Arena_Memory) {
    .arena = a,
    .cursor = a->cursor,
  };
}
void temp_arena_memory_end(Temorary_Arena_Memory tmp_mem){
  tmp_mem.arena->cursor = tmp_mem.cursor;
}


#if defined(DG_ARENA_DEBUG)
#define arena_alloc(arena, size) _tracking_arena_alloc(arena, size, DEFAULT_ALIGNMENT, __FILE__, __LINE__)
#define arena_alloc_pass_loc(arena, size, file, line) _tracking_arena_alloc(arena, size, DEFAULT_ALIGNMENT, file, line)
#else
#define arena_alloc(arena, size) _arena_alloc(arena, size, DEFAULT_ALIGNMENT)
#define arena_alloc_pass_loc(arena, size, file, line) _arena_alloc(arena, size, DEFAULT_ALIGNMENT)
#endif //DG_ARENA_DEBUG

// TODO:
void arena_clear(Arena *arena) {
  arena->cursor = 0;
}

void arena_release(Arena arena) {
  free(arena.data);
}

#endif
// }}}

// dynamic array and other containers...? {{{
#ifndef CDG_CONTAINER_C
#define CDG_CONTAINER_C
// TODO: include arena.c

// NOTE: implementation from https://nullprogram.com/blog/2023/10/05/

// TODO: usar ou remover
typedef struct dynamic_array_header {
  i32 len;
  i32 cap;
} Dynamic_Array_Header;

#define Make_Dynamic_Array_type(type) \
struct { \
  type *data; \
  i32 len; \
  i32 cap; \
}

typedef Make_Dynamic_Array_type(void) _Any_Dynamic_Array;

void dynamic_array_grow(_Any_Dynamic_Array *arr, Arena *a, u32 item_size) {
  _Any_Dynamic_Array replica = {};
  memcpy(&replica, arr, sizeof(replica));

  if (!replica.data) {
    // TODO: default capacity
    replica.cap = 1;
    replica.data = arena_alloc(a, 2 * item_size * replica.cap);
  } else if ((replica.data + replica.cap * item_size) == (a->data + a->cursor)) {
    // NOTE: se a última alocação da arena foi esse array,
    // então da para extender allocando um `cap` a mais
    arena_alloc(a, item_size * replica.cap);
  } else {
    void *data = arena_alloc(a, 2 * item_size * replica.cap);
    memcpy(data, replica.data, item_size*replica.len);
    replica.data = data;
  }

  replica.cap *= 2;
  memcpy(arr, &replica, sizeof(replica));

}

#define dynamic_array_push(arr, item, arena) DG_STATEMENT \
  ( \
    if ((arr)->len >= (arr)->cap) { \
      dynamic_array_grow((_Any_Dynamic_Array*)(arr), (arena), sizeof(*(arr)->data)); /* NOLINT */\
    }\
    (arr)->data[(arr)->len++] = (item); \
  )

void _dynamic_array_clear(_Any_Dynamic_Array *arr) {
  arr->len = 0;
}

#define dynamic_array_clear(arr) _dynamic_array_clear((_Any_Dynamic_Array *) (arr))

#define Make_Slice_Type(type) \
struct { \
  type *data; \
  i32 len; \
}

typedef Make_Slice_Type(void) _Any_Slice;

#define make_slice(arena, slice, len) dg_make_slice(arena, (_Any_Slice *)slice, len, sizeof(*(slice)->data))

void dg_make_slice(Arena *a, _Any_Slice *slice, u64 len, u64 item_size){
  _Any_Slice res = {};

  void *data = arena_alloc(a, len * item_size);

  if (data) {
    res.len = len;
    res.data = data;
  }

  *slice = res;
}

#define SLICE_AT(slice, idx) \
  (slice.data[idx < 0 ? slice.len + idx : idx])

#endif // CDG_CONTAINER_C
// }}}

// Matrix types and operations {{{
#ifndef CDG_MATRIX_H
#define CDG_MATRIX_H

typedef struct {
  f32 *data;
  i32 rows, cols;
} DG_Matrix;

#define MAT_AT(mat, row, col) (mat).data[row*(mat).cols + col]

DG_Matrix matrix_alloc(Arena *a, u32 rows, u32 cols){
  DG_Matrix m = {
    .rows = rows,
    .cols = cols,
  };
  void* data = arena_alloc(a, sizeof(*m.data) * rows * cols);
  m.data = data;
  return m;
}

void matrix_copy(DG_Matrix dst, DG_Matrix src){
  DG_ASSERT(src.rows == dst.rows);
  DG_ASSERT(src.cols == dst.cols);

  memcpy(dst.data, src.data, sizeof(*src.data) * src.rows * src.cols);
}

// NOTE: não sei como me sinto passando um valor por cópia mesmo sabendo que ele vai ser modificado
void matrix_fill(DG_Matrix m, f32 val) {
  for(u32 r = 0; r < m.rows; ++r) {
    for(u32 c = 0; c < m.cols; ++c) {
      MAT_AT(m, r, c) = val;
    }
  }
}

void matrix_sum_in_place(DG_Matrix dst, DG_Matrix a, DG_Matrix b) {
  DG_ASSERT(a.rows == b.rows);
  DG_ASSERT(a.cols == b.cols);

  for(u32 r = 0; r < dst.rows; ++r) {
    for(u32 c = 0; c < dst.cols; ++c) {
        MAT_AT(dst, r, c) = MAT_AT(a, r, c) + MAT_AT(b, r, c);
    }
  }
}

void matrix_dot_in_place(DG_Matrix dst, DG_Matrix a, DG_Matrix b) {
  DG_ASSERT(a.cols == b.rows);
  DG_ASSERT(dst.cols == b.cols);
  DG_ASSERT(dst.rows == a.rows);

  for(u32 r = 0; r < dst.rows; ++r) {
    for(u32 c = 0; c < dst.cols; ++c) {
      f32 val = 0;
      for (u32 k = 0; k < a.cols; ++k) {
         val += MAT_AT(a, r, k) * MAT_AT(b, k, c);
      }
      MAT_AT(dst, r, c) = val;
    }
  }
}

DG_Matrix matrix_dot(Arena *arena, DG_Matrix a, DG_Matrix b) {
  DG_Matrix res = matrix_alloc(arena, a.rows, b.cols);
  matrix_dot_in_place(res, a, b);
  return res;
}

void dg_matrix_print(DG_Matrix m, char *name) {
  printf("%s = [ \n", name);
  for(u32 r = 0; r < m.rows; ++r) {
    printf("    ");
    for(u32 c = 0; c < m.cols; ++c) {
      printf(" %f", MAT_AT(m, r, c));
    }
    printf("\n");
  }
  printf("]\n");
}
#define matrix_print(x) dg_matrix_print(x, STR(x))


#endif // CDG_MATRIX_H
// }}}

// math {{{
#ifndef CDG_MATH_H
#define CDG_MATH_H

#include <math.h>
f32 sigmoidf(f32 x) {
  return 1.f / (1.f + expf(-x));
}

f32 randf(){
  return (f32) rand() / (f32) RAND_MAX;
}

#endif // CDG_MATH_H
// }}}

