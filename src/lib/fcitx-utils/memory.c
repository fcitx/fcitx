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
    void* memory;
    size_t size;
    size_t freeOff;
} FcitxMemoryChunk;

struct _FcitxMemoryPool {
    UT_array* fullchunks;
    UT_array* chunks;
};

static void fcitx_memory_chunk_free(void* c);
const UT_icd chunk_icd = { sizeof(FcitxMemoryChunk), NULL, NULL, fcitx_memory_chunk_free};

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

FCITX_EXPORT_API
void* fcitx_memory_pool_alloc(FcitxMemoryPool* pool, size_t size)
{
    FcitxMemoryChunk* chunk;
    for(chunk = (FcitxMemoryChunk*) utarray_front(pool->chunks);
        chunk != NULL;
        chunk = (FcitxMemoryChunk*) utarray_next(pool->chunks, chunk))
    {
        if (chunk->freeOff + size <= chunk->size)
            break;
    }
    
    if (chunk == NULL) {
        size_t chunkSize = ((size + FCITX_MEMORY_POOL_PAGE_SIZE - 1) / FCITX_MEMORY_POOL_PAGE_SIZE) * FCITX_MEMORY_POOL_PAGE_SIZE;
        FcitxMemoryChunk c;
        c.freeOff = 0;
        c.size = chunkSize;
        c.memory = fcitx_utils_malloc0(chunkSize);
        
        utarray_push_back(pool->chunks, &c);
        chunk = (FcitxMemoryChunk*) utarray_back(pool->chunks);
    }
    
    void* result = chunk->memory + chunk->freeOff;
    chunk->freeOff += size;
    
    if (chunk->size - chunk->freeOff <= FCITX_MEMORY_CHUNK_FULL_SIZE)
    {
        utarray_push_back(pool->fullchunks, chunk);
        int idx = utarray_eltidx(pool->chunks, chunk);
        utarray_remove_quick(pool->chunks, idx);
    }
    
    return result;
}

FCITX_EXPORT_API
void fcitx_memory_pool_destroy(FcitxMemoryPool* pool)
{
    utarray_free(pool->fullchunks);
    utarray_free(pool->chunks);
    free(pool);
}
