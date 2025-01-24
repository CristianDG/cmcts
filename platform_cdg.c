// types.c {{{
#ifndef CDG_TYPES_H
#define CDG_TYPES_H

#include <stdint.h>
#include <stdbool.h>
// TODO: trocar pelo meu assert
#include <assert.h>

#define MAX(x, y) (x >= y ? x : y)
#define MIN(x, y) (x <= y ? x : y)

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float  f32;
typedef double f64;

typedef uintptr_t uintptr;
typedef intptr_t  intptr;

typedef u32 b32;

#define KILOBYTE 1024
#define MEGABYTE 1048576

#endif
// }}}

// alloc.c {{{
#ifndef CDG_ALLOC_C
#define CDG_ALLOC_C

#include <stdlib.h>
#include <stdio.h>
#include <string.h> // for memset

typedef struct {
  u8 *data;
  // TODO: add alignment
  // TODO: add temp_count
  u32 size;
  u32 cursor;
} Arena;

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

Temorary_Arena_Memory temp_arena_memory_begin(Arena *a){
  return (Temorary_Arena_Memory) {
    .arena = a,
    .cursor = a->cursor,
  };
}
void temp_arena_memory_end(Temorary_Arena_Memory tmp_mem){
  tmp_mem.arena->cursor = tmp_mem.cursor;
}

// TODO: add alignment
void *_arena_alloc(Arena *arena, uintptr size) {
  // acredito que um if é melhor que um assert aqui
  if (arena->cursor + size > arena->size) {
    return 0;
  }
  void *ptr = &arena->data[arena->cursor];
  memset(ptr, 0, size);
  arena->cursor += size;
  return ptr;
}

// TODO: olhar https://youtu.be/443UNeGrFoM?si=DBJXmKB_z8W8Yrrf&t=3074
void *_debug_arena_alloc(Arena *arena, uintptr size, char *file, i32 line) {
  // TODO: registrar onde foram todas as alocações
  void *ptr = _arena_alloc(arena, size);
  if (ptr == 0) {
    fprintf(stderr, "%s:%d Could not allocate %zu bytes\n", file, line, size);
  }
  return ptr;
}


#if defined(DG_ARENA_DEBUG)
#define arena_alloc(arena, size) _debug_arena_alloc(arena, size, __FILE__, __LINE__)
#define arena_alloc_pass_loc(arena, size, file, line) _debug_arena_alloc(arena, size, file, line)
#else
#define arena_alloc(arena, size) _arena_alloc(arena, size)
#define arena_alloc_pass_loc(arena, size, file, line) _arena_alloc(arena, size)
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

typedef struct dynamic_array_header {
  u32 len;
  u32 cap;
} Dynamic_Array_Header;

#define Dynamic_Array(type) \
struct { \
  u32 len; \
  u32 cap; \
  type *data; \
}

typedef Dynamic_Array(void) Any_Dynamic_Array;

void dynamic_array_grow(Any_Dynamic_Array *arr, Arena *a, u32 item_size) {
  Any_Dynamic_Array replica = {};
  memcpy(&replica, arr, sizeof(replica));

  replica.cap = replica.cap ? replica.cap : 1;
  void *data = arena_alloc(a, 2 * item_size * replica.cap);
  replica.cap *= 2;
  if (replica.len) {
    memcpy(data, replica.data, item_size*replica.len);
  }
  replica.data = data;

  memcpy(arr, &replica, sizeof(replica));
}

#define dynamic_array_push(arr, item, arena) \
  do { \
    if ((arr)->len >= (arr)->cap) { \
      dynamic_array_grow((Any_Dynamic_Array*)arr, arena, sizeof(*(arr)->data)); /* NOLINT */\
    }\
    (arr)->data[(arr)->len++] = item; \
  } while (0)

void _dynamic_array_clear(Any_Dynamic_Array *arr) {
  arr->len = 0;
}

#define dynamic_array_clear(arr) _dynamic_array_clear((Any_Dynamic_Array *) arr)



#endif
// }}}
