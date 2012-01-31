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

#ifndef _FCITX_MEMORY_H_
#define _FCITX_MEMORY_H_

#include <stdlib.h>

/**
 * @file memory.h
 * 
 * a simple naive memory pool for fcitx, scenerio is many small allocate,
 * but nearly no single memory free, but it may only free all the allocated memory
 *
 */

typedef struct _FcitxMemoryPool FcitxMemoryPool;

/**
 * @brief create a memroy pool
 *
 * @return FcitxMemoryPool*
 **/
FcitxMemoryPool* fcitx_memory_pool_create();

/**
 * @brief allocate piece of memory from pool
 *
 * @param pool memory pool
 * @param size size of wanted memory
 * @return void*
 **/
void* fcitx_memory_pool_alloc(FcitxMemoryPool* pool, size_t size);

/**
 * @brief free memory pool and free all the memory inside the pool
 *
 * @param pool memory ppol
 * @return void
 **/
void fcitx_memory_pool_destroy(FcitxMemoryPool* pool);

#endif
