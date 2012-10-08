/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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

#ifndef X11HANDLERTABLE_H
#define X11HANDLERTABLE_H

typedef struct _FcitxHandlerTable FcitxHandlerTable;
/**
 * Function used to free internal data of a structure
 * DO NOT free the pointer itself
 **/
#define INVALID_ID ((unsigned int)-1)
typedef void (*FcitxFreeContentFunc)(void*);

FcitxHandlerTable *fcitx_handler_table_new(size_t obj_size,
                                           FcitxFreeContentFunc free_func);
unsigned int fcitx_handler_table_append(FcitxHandlerTable *table,
                                        size_t keysize, const void *key,
                                        const void *obj);
unsigned int fcitx_handler_table_prepend(FcitxHandlerTable *table,
                                         size_t keysize, const void *key,
                                         const void *obj);
void *fcitx_handler_table_get_by_id(FcitxHandlerTable *table, unsigned int id);
void *fcitx_handler_table_first(FcitxHandlerTable *table, size_t keysize,
                                const void *key);
void *fcitx_handler_table_last(FcitxHandlerTable *table, size_t keysize,
                               const void *key);
void *fcitx_handler_table_next(FcitxHandlerTable *table, const void *obj);
void *fcitx_handler_table_prev(FcitxHandlerTable *table, const void *obj);
unsigned int fcitx_handler_table_first_id(FcitxHandlerTable *table,
                                          size_t keysize, const void *key);
unsigned int fcitx_handler_table_last_id(FcitxHandlerTable *table,
                                         size_t keysize, const void *key);
unsigned int fcitx_handler_table_next_id(FcitxHandlerTable *table,
                                         const void *obj);
unsigned int fcitx_handler_table_prev_id(FcitxHandlerTable *table,
                                         const void *obj);
void fcitx_handler_table_remove_key(FcitxHandlerTable *table, size_t keysize,
                                    const void *key);
void fcitx_handler_table_remove_by_id(FcitxHandlerTable *table, unsigned int id);
void fcitx_handler_table_free(FcitxHandlerTable *table);

#endif
