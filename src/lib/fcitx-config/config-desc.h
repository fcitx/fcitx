/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
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

#ifndef _FCITX_CONFIG_DESC_H_
#define _FCITX_CONFIG_DESC_H_

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <fcitx-utils/utils.h>
#include <fcitx-config/hotkey.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* MOVE TO fcitx-utils */
    /* obj is evaluated more than once but I cannot find a C99&C++0x
     * compatible way to avoid this. */
#define FCITX_CHECK_MEMBER(obj, size, member)                           \
    ((size) > (((void*)&(obj)->member) - ((void*)(obj))) ? true : false)
#define _FCITX_GET_MEMBER(obj, size, member, default_val, ...)          \
    (FCITX_CHECK_MEMBER(obj, size, member) ? (obj)->member : default_val)
#define FCITX_GET_MEMBER(obj, size, member, default_val...)     \
    _FCITX_GET_MEMBER(obj, size, member, ##default_val, NULL)

    typedef struct _FcitxConfigDescBackend FcitxConfigDescBackend;
    typedef struct _FcitxConfigDescVTable FcitxConfigDescVTable;
    typedef struct _FcitxConfigDescGroupVTable FcitxConfigDescGroupVTable;
    typedef struct _FcitxConfigDescOptionVTable FcitxConfigDescOptionVTable;

    typedef enum {
        T_Invalid,
        T_Integer,
        T_Color,
        T_String,
        T_Char,
        T_Boolean,
        T_Enum,
        T_File,
        T_Hotkey,
        T_Font,
        T_I18NString
    } FcitxConfigType;

    typedef struct {
        struct {
            int min;
            int max;
        } integerConstrain;

        struct {
            size_t maxLength;
        } stringConstrain;

        struct {
            boolean disallowNoModifer;
            boolean allowModifierOnly;
        } hotkeyConstrain;

        struct {
            int enumCount; /**< length of enumDesc */
            char **enumDesc; /**< enum string description, a user visble string */
        } enumConstrain;

        void *_padding[10];
    } FcitxConfigConstrain;

    typedef struct {
        double r; /**< red */
        double g; /**< green */
        double b; /**< blue */
    } FcitxConfigColor;

    typedef struct {
        FcitxConfigType type;
        union {
            intptr_t integer;
            boolean boolvalue;
            FcitxHotkey *hotkey;
            FcitxConfigColor *color;
            int enumerate;
            char *string;
            char chr;
        };
    } FcitxConfigValue;

    typedef struct {
        FcitxConfigDescBackend *backend;
        FcitxConfigDescVTable *vtable;
    } FcitxConfigDesc;

    typedef struct {
        FcitxConfigDescGroupVTable *vtable;
        char *name;
    } FcitxConfigDescGroup;

    typedef struct {
        FcitxConfigDescOptionVTable *vtable;
        boolean readonly;
        char *name;
        char *desc;
        FcitxConfigValue default_value;
        FcitxConfigConstrain constrain;
        boolean advance;
        char *long_desc;
    } FcitxConfigDescOption;

    typedef struct {
        void (*unref)();
        FcitxConfigDesc *(*load)();
    } FcitxConfigDescBackend;

    struct _FcitxConfigDescVTable {
        size_t vtable_size;
        size_t struct_size;
        void (*ref)();
        void (*unref)();
        FcitxConfigDescGroup *(*get_group)();
        FcitxConfigDescOption *(*get_option)();
    };

    struct _FcitxConfigDescGroupVTable {
        size_t vtable_size;
        size_t struct_size;
        void (*ref)();
        void (*unref)();
        FcitxConfigDescOption *(*get_option)();
        // ?? get_desc?
    };

    struct _FcitxConfigDescOptionVTable {
        size_t vtable_size;
        size_t struct_size;
        void (*ref)();
        void (*unref)();
        // ?? get_desc?
    };

    typedef struct _FcitxConfigManager FcitxConfigManager;

    // priority (or in struct)
    int _fcitx_config_reg_desc_backend(FcitxConfigManager *mgr,
                                       const FcitxConfigDescBackend *backend,
                                       size_t struct_size, void *data);
#define __fcitx_config_reg_desc_backend(mgr, backend, data, ...)        \
    _fcitx_config_reg_desc_backend(mgr, backend,                        \
                                   (sizeof(FcitxConfigDescBackend)), data)
#define fcitx_config_reg_desc_backend(mgr, backend, data...)    \
    __fcitx_config_reg_desc_backend(mgr, backend, ##data, NULL)

    // Wrappers

#ifdef __cplusplus
}
#endif

#endif
