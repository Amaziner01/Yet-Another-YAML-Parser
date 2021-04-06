#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stdio.h>
#include <string.h>

#define inline_ static inline

#define RET_ERR(msg, ret) { fprintf(stderr, "%s\n", msg); return ret; }

typedef unsigned long long u64_t;
typedef unsigned int u32_t;
typedef unsigned char u8_t;

struct header {
    struct header *next;
    u32_t size;
    u8_t freed;
};

typedef struct header header_t;

struct mempool {
    void *mem_block;
    u32_t mem_size;
    u32_t mem_used_bytes;
};

typedef struct mempool mempool_t;

inline_ mempool_t *
create_mempool( u32_t size ) {
    mempool_t *m = (mempool_t*)malloc(sizeof(mempool_t)); 
    if (!m) return NULL;

    m->mem_block = malloc(size);
    if (!m->mem_block) {
        free(m);
        return NULL;
    }

    memset(m->mem_block, 0, size);

    m->mem_used_bytes = 0;
    m->mem_size = size;

    return m;
}

inline_ void
terminate_mempool( mempool_t *mempool ) {
    if (!mempool) return;

    free(mempool->mem_block);
    free(mempool);
}

inline_ void
print_mem( void *mem, u32_t size ) {
    if (!mem) return;

    printf("Address: %p\n", mem);

    u8_t *ptr = (u8_t*)mem;
    for (u32_t i = 0; i < size; i++) {
        
        if (i % 16 == 0) {
        puts("");
        printf("%iL: \t", i);
        }

        printf("%02X ", *ptr);
        ptr++;
    }
    puts("\n");
}

inline_ void *
alloc( u32_t size, mempool_t *mempool ) {
    if (!mempool) return NULL;
    if (size == 0) return NULL;
    

    header_t *last_header = (header_t*)mempool->mem_block;
    header_t *header;

    int final_size = size;
    
    if (!last_header->size) {
        header = last_header;
        goto assignment;
    }

    while (last_header->next) {
        if (last_header->freed) {
            if (last_header->size >= size) {
                header = last_header;
                goto assignment; 
            } 
        }

        last_header = last_header->next;
    }

    final_size += sizeof(header_t);
    header = (header_t*)((u64_t)last_header + sizeof(header_t) + last_header->size);

    last_header->next = header;
    header->next = NULL;

assignment:
    mempool->mem_used_bytes += final_size;

    if (mempool->mem_used_bytes + final_size > mempool->mem_size) {
        last_header->next = NULL;
        RET_ERR("Not enough memory to allocate", NULL);
    }

    header->freed = 0;
    header->size = size;

    return (void*)((u64_t)header + sizeof(header_t));
}

inline_ void
release( void *ptr, mempool_t *mempool ) {
    if(!ptr) return;
    if (!mempool) return;

    header_t *header = (header_t*)((u64_t)ptr - sizeof(header_t));
    header->freed = 1;
}

#undef inline_
#undef THROW_ERR

#endif /* MEMPOOL_H */