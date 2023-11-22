
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#define PRE_ALLOC_SIZE 20000
#define SIZE_ALIGN 32
#define HEADER_SIZE 32
#define MIN_CHUNK_SIZE 64

typedef struct block block_t;
struct block {
    size_t size;
    int free;
    block_t *prev, *next;
};

static void* pool = NULL;
static block_t* head = NULL;

static void _prealloc() {
    pool = mmap(NULL, PRE_ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 0, 0);
    if (pool == MAP_FAILED)
        perror("mmap"), exit(EXIT_FAILURE);
    head = pool;
    head->size = PRE_ALLOC_SIZE - HEADER_SIZE;
    head->free = 1;
    head->prev = head->next = NULL;
}

static void _dealloc() {
    if (munmap(pool, PRE_ALLOC_SIZE) < 0)
        perror("munmap"), exit(EXIT_FAILURE);
    pool = NULL;
    head = NULL;
}

static void _print() {
}

static block_t* _best_fit(size_t size) {
    block_t* best = NULL;
    for (block_t* it = head; it != NULL; it = it->next) {
        if (it->size >= size && (best == NULL || it->size < best->size)) {
            best = it;
        }
    }
    return best;
}

static void link_block(block_t* prev, block_t* next) {
    prev->next = next;
    next->prev = prev;
}

static block_t* _alloc_block(size_t size) {
    block_t* it = _best_fit(size);
    if (it == NULL) return NULL;
    if (it->size >= size + MIN_CHUNK_SIZE) {
        block_t* next = (block_t*)((char*)it + HEADER_SIZE + size);
    }
    return it;
}

static void _free_block(block_t* ptr) {
    // TODO
}

void* malloc(size_t size) {
    if (size == 0) {
        _print();
        _dealloc();
        return NULL;
    }
    if (pool == NULL) _prealloc();
    size_t size_align = ((size - 1) / SIZE_ALIGN + 1) * SIZE_ALIGN;
    return (char*)_alloc_block(size_align) + HEADER_SIZE;
}

void free(void* ptr) {
    return _free_block((block_t*)((char*)ptr - HEADER_SIZE));
}
