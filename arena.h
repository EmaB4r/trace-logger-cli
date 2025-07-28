#include <stddef.h>
#include <stdint.h>

#define ARENA_SIZE (128 * 1024)
#define ARENA_ALIGN 64

typedef struct {
    uint8_t buffer[ARENA_SIZE] __attribute__((aligned(64)));
    size_t index;
} arena_t;

void * arena_malloc(size_t s);

void arena_free(void * p);


inline void arena_reset();