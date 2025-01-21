// types.c {{{
#ifndef CDG_TYPES_H
#define CDG_TYPES_H

#include <stdint.h>
#include <stdbool.h>

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

Arena arena_init_malloc(u32 size) {
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
void *arena_alloc(Arena *arena, uintptr size) {
  if (arena->cursor + size > arena->size) {
    fprintf(stderr, "Could not allocate %zu bytes\n", size);
    return 0;
  }
  void *ptr = &arena->data[arena->cursor];
  memset(ptr, 0, size);
  arena->cursor += size;
  return ptr;
}

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

#include <assert.h>

// NOTE: implementation from https://www.youtube.com/watch?v=_KSKH8C9Gf0 and https://nullprogram.com/blog/2023/10/05/
typedef struct {
  u32 len;
  u32 cap;
  u32 item_size;
  u8  magic[4];
} Dynamic_Array_Header;

// TODO: assert header exists
Dynamic_Array_Header *dynamic_array_header(void *arr) {
  Dynamic_Array_Header *res = arr - sizeof(Dynamic_Array_Header);
  assert(res->magic[0] == 'D');
  assert(res->magic[1] == 'Y');
  assert(res->magic[2] == 'N');
  assert(res->magic[3] == 0);
  return res;
}


void dynamic_array_grow(void **arr, Arena *a){
  Dynamic_Array_Header *header = dynamic_array_header(*arr);
  assert(header->len >= header->cap);
  void *new_place = arena_alloc(a, sizeof(Dynamic_Array_Header) + (header->item_size * header->cap * 2));
  header->cap *= 2;
  memcpy(new_place, header, sizeof(Dynamic_Array_Header) + (header->item_size * header->cap));
  *arr = new_place + sizeof(Dynamic_Array_Header);
}


void dynamic_array_clear(void *arr){
  Dynamic_Array_Header *header = dynamic_array_header(arr);
  header->len = 0;
}

void _dynamic_array_make(Arena *a, void **arr, u32 initial_capacity, u32 item_size){
  Dynamic_Array_Header *header = arena_alloc(a, sizeof(Dynamic_Array_Header) + (item_size * initial_capacity));

  *header = (Dynamic_Array_Header) {
    .item_size = item_size,
    .cap = initial_capacity,
    .len = 0,
    .magic = "DYN",
  };

  *arr = header + 1;
}

#define dynamic_array_make(arena, dyn, cap) _dynamic_array_make(arena, (void **)dyn, cap, sizeof(*dyn))

u32 dynamic_array_len(void *arr) {
  Dynamic_Array_Header *header = dynamic_array_header(arr);
  return header->len;
}

void _dynamic_array_push(void **arr, Arena *a, void* item, u32 item_size){
  if (*arr == 0) {
    _dynamic_array_make(a, arr, 32, item_size);
  }
  Dynamic_Array_Header *header = dynamic_array_header(*arr);
  if (header->len >= header->cap) {
    dynamic_array_grow(arr, a);
  }

  void *addr = ((void*)*arr) + (header->len++ * header->item_size);
  memcpy(addr, item, item_size);
}

#define dynamic_array_push(dyn, item, arena) _dynamic_array_push((void **)dyn, arena, &item, sizeof(**dyn)) /* NOLINT */

#endif
// }}}
