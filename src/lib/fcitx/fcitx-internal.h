/***************************************************************************
 *   Copyright (C) 2011~2012 by CSSlayer                                   *
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
#ifndef _FCITX_INTERNAL_H_
#define _FCITX_INTERNAL_H_

/**
 * @file fcitx-internal.h
 * @author CS Slayer <wengxt@gmail.com>
 * some misc definition for Fcitx
 */

#ifdef __cplusplus
extern "C" {
#endif

#define FCITX_GETTER_REF(type, property, name, return_type) \
    FCITX_EXPORT_API \
    return_type* type##Get##property(type* object) \
    { \
        return &object->name; \
    }

#define FCITX_GETTER_VALUE(type, property, name, return_type) \
    FCITX_EXPORT_API \
    return_type type##Get##property(type* object) \
    { \
        return object->name; \
    }

#define FCITX_SETTER(type, property, name, return_type) \
    FCITX_EXPORT_API \
    void type##Set##property(type* object, return_type value) \
    { \
        object->name = value; \
    }

#ifdef __cplusplus
}
#endif

#endif/*_FCITX_H_*/

// kate: indent-mode cstyle; space-indent on; indent-width 0;
