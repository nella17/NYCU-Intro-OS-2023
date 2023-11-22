
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>
#define PRE_ALLOC_SIZE 20000
#define SIZE_ALIGN 32
#define HEADER_SIZE 32
#define MIN_DATA_SIZE 32

// #define DEBUG
static char buf[1024];

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
    size_t max_size = 0;
    for (block_t* it = head; it != NULL; it = it->next) {
        if (it->free && (it->size > max_size))
            max_size = it->size;
    }
    write(1, buf, sprintf(buf, "Max Free Chunk Size = %lu\n", max_size));
}

static void _print_blocks() {
#ifdef DEBUG
    write(1, buf, sprintf(buf, "head = %p\n", head));
    for (block_t* it = head; it != NULL; it = it->next) {
        write(1, buf, sprintf(buf, " it = %p\t%p <--\t-->%p\n  size = %lx\tfree = %d\n", it, it->prev, it->next, it->size, it->free));
    }
    write(1, buf, sprintf(buf, "\n"));
#endif
}

static block_t* _first_fit(size_t size) {
    for (block_t* it = head; it != NULL; it = it->next)
        if (it->free && it->size >= size)
            return it;
    return NULL;
}

static void _link_block(block_t* prev, block_t* next) {
    if (prev) prev->next = next;
    if (next) next->prev = prev;
}

static block_t* _alloc_block(size_t size) {
    block_t* it = _first_fit(size);
    if (it == NULL) return NULL;
    ssize_t remain = it->size - size - HEADER_SIZE;
    if (remain >= MIN_DATA_SIZE) {
        it->size = size;
        block_t* next = (block_t*)((char*)it + HEADER_SIZE + size);
        next->free = 1;
        next->size = remain;
        _link_block(next, it->next);
        _link_block(it, next);
    }
    it->free = 0;
    _print_blocks();
    return it;
}

static void _merge_prev_block(block_t* ptr) {
    if (!ptr || !ptr->free || !ptr->prev || !ptr->prev->free) return;
    ptr->prev->size += HEADER_SIZE + ptr->size;
    _link_block(ptr->prev, ptr->next);
}

static void _free_block(block_t* ptr) {
    ptr->free = 1;
    _merge_prev_block(ptr->next);
    _merge_prev_block(ptr);
    _print_blocks();
}

void* malloc(size_t size) {
#ifdef DEBUG
    write(1, buf, sprintf(buf, "malloc(0x%lx)\n", size));
#endif
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
#ifdef DEBUG
    write(1, buf, sprintf(buf, "free(%p)\n", ptr));
#endif
    if (!ptr) return;
    return _free_block((block_t*)((char*)ptr - HEADER_SIZE));
}
