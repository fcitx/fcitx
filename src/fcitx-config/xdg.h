/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** 
 * @file xdg.h
 * @brief XDG Related path handle
 * @author CSSlayer
 * @version 4.0.0
 * @date 2010-05-02
 */

#ifndef XDG_H
#define XDG_H

#include <stdio.h>
#include <X11/Xlib.h>

FILE *GetLibFile(const char *filename, const char *mode, char **retFile);
FILE *GetXDGFile(const char *fileName, char **path, const char *mode, size_t len, char **retFile);
char **GetXDGPath(
        size_t *len,
        const char* homeEnv,
        const char* homeDefault,
        const char* suffixHome,
        const char* dirsDefault,
        const char* suffixGlobal);

FILE *GetXDGFileData(const char *fileName, const char *mode, char**retFile);
FILE *GetXDGFileUser(const char *fileName, const char *mode, char **retFile);
FILE *GetXDGFileTable(const char *fileName, const char *mode, char **retFile, Bool forceUser);
FILE *GetXDGFilePinyin(const char *fileName, const char *mode, char **retFile);
void FreeXDGPath(char **path);
#endif
