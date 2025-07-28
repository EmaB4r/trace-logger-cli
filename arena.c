#include "arena.h"

arena_t global_arena = {0};

void* arena_malloc(size_t size) {
    size = (size + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
    if(global_arena.index + size > ARENA_SIZE) return NULL;
    void* ptr = &global_arena.buffer[global_arena.index];
    global_arena.index += size;
    return ptr;
}

void arena_free(void * p){}

void arena_reset(){
    global_arena.index=0;
} 