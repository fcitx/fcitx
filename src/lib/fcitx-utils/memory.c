/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"
#include "fcitx-utils/memory.h"
#include "utarray.h"
#include "utils.h"

/* use 8k as pagesize */
#define FCITX_MEMORY_POOL_PAGE_SIZE (8*1024)
#define FCITX_MEMORY_CHUNK_FULL_SIZE (16)

typedef struct _FcitxMemoryChunk {
    void *cur;
    void *end;
    void *memory;
} FcitxMemoryChunk;

struct _FcitxMemoryPool {
    UT_array* fullchunks;
    UT_array* chunks;
};

static void fcitx_memory_chunk_free(void* c);
static const UT_icd chunk_icd = {
    sizeof(FcitxMemoryChunk), NULL, NULL, fcitx_memory_chunk_free
};

void fcitx_memory_chunk_free(void* c) {
    FcitxMemoryChunk* chunk = (FcitxMemoryChunk*) c;
    if (chunk->memory) {
        free(chunk->memory);
        chunk->memory = NULL;
    }
}

FCITX_EXPORT_API
FcitxMemoryPool* fcitx_memory_pool_create()
{
    FcitxMemoryPool* pool = fcitx_utils_malloc0(sizeof(FcitxMemoryPool));
    utarray_new(pool->fullchunks, &chunk_icd);
    utarray_new(pool->chunks, &chunk_icd);
    return pool;
}

static inline void*
memory_align_ptr(void *p)
{
    return (void*)fcitx_utils_align_to((uintptr_t)p, sizeof(int));
}

FCITX_EXPORT_API
void* fcitx_memory_pool_alloc_align(FcitxMemoryPool* pool, size_t size, int align)
{
    FcitxMemoryChunk* chunk;
    void *result;
    for(chunk = (FcitxMemoryChunk*) utarray_front(pool->chunks);
        chunk != NULL;
        chunk = (FcitxMemoryChunk*) utarray_next(pool->chunks, chunk)) {
        result = align ? memory_align_ptr(chunk->cur) : chunk->cur;
        void *new = result + size;
        if (new <= chunk->end) {
            chunk->cur = new;
            break;
        }
    }

    if (chunk == NULL) {
        size_t chunkSize = ((size + FCITX_MEMORY_POOL_PAGE_SIZE - 1) / FCITX_MEMORY_POOL_PAGE_SIZE) * FCITX_MEMORY_POOL_PAGE_SIZE;
        FcitxMemoryChunk c;
        /* should be properly aligned already */
        result = fcitx_utils_malloc0(chunkSize);
        c.end = result + chunkSize;
        c.memory = result;
        c.cur = result + size;

        utarray_push_back(pool->chunks, &c);
        chunk = (FcitxMemoryChunk*)utarray_back(pool->chunks);
    }

    if (chunk->end - chunk->cur <= FCITX_MEMORY_CHUNK_FULL_SIZE) {
        utarray_push_back(pool->fullchunks, chunk);
        unsigned int idx = utarray_eltidx(pool->chunks, chunk);
        utarray_remove_quick(pool->chunks, idx);
    }

    return result;
}

#undef fcitx_memory_pool_alloc

FCITX_EXPORT_API
void* fcitx_memory_pool_alloc(FcitxMemoryPool* pool, size_t size)
{
    return fcitx_memory_pool_alloc_align(pool, size, 0);
}

FCITX_EXPORT_API
void fcitx_memory_pool_destroy(FcitxMemoryPool* pool)
{
    utarray_free(pool->fullchunks);
    utarray_free(pool->chunks);
    free(pool);
}

FCITX_EXPORT_API
void
fcitx_memory_pool_clear(FcitxMemoryPool *pool)
{
    utarray_clear(pool->fullchunks);
    utarray_clear(pool->chunks);
}
