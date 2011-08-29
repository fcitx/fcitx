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

#ifndef _FCITX_XDG_H_
#define _FCITX_XDG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Get library file
     *
     * @param filename filename
     * @param mode file open mode
     * @param retFile return file name
     * @return FILE*
     **/
    FILE *GetLibFile(const char *filename, const char *mode, char **retFile);
    /**
     * @brief get a xdg file pointer with given path, if mode contains "w", it will create necessary parent folder
     *
     * @param fileName filename
     * @param path returns by GetXDGPath
     * @param mode file open mode
     * @param len length of path
     * @param retFile return file name
     * @return FILE*
     *
     * @see GetXDGPath
     **/
    FILE *GetXDGFile(const char *fileName, char **path, const char *mode, size_t len, char **retFile);
    /**
     * @brief get xdg path with given arguement
     *
     * @param len return array size
     * @param homeEnv homeEnv
     * @param homeDefault homeDefault
     * @param suffixHome suffixHome
     * @param dirsDefault dirsDefault
     * @param suffixGlobal suffixGlobal
     * @return char**
     **/
    char **GetXDGPath(
        size_t *len,
        const char* homeEnv,
        const char* homeDefault,
        const char* suffixHome,
        const char* dirsDefault,
        const char* suffixGlobal);

    /**
     * @brief get xdg file with prefix string, usually [install_prefix]/fcitx/prefix/filename and ~/.config/fcitx/prefix/filename
     *
     * @param prefix prefix
     * @param fileName filename
     * @param mode file open mode
     * @param retFile file name to return
     * @return FILE*
     *
     * @see GetXDGFile
     **/
    FILE *GetXDGFileWithPrefix(const char* prefix, const char *fileName, const char *mode, char**retFile);
    /**
     * @brief get xdg file with prefix string, usually ~/.config/fcitx/prefix/filename
     *
     * @param prefix prefix
     * @param fileName filename
     * @param mode file open mode
     * @param retFile file name to return
     * @return FILE*
     *
     * @see GetXDGFile
     **/
    FILE *GetXDGFileUserWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile);
    /**
     * @brief free xdg path return by GetXDGPath
     *
     * @param path path array
     * @return void
     * @see GetXDGFile
     **/
    void FreeXDGPath(char **path);

#ifdef __cplusplus
}

#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
