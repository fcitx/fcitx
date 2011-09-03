/***************************************************************************
 *   Copyright (C) 20010~2010 by CSSlayer                                  *
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
 * @file xdg.c
 * @brief xdg related path handle
 * @author CSSlayer
 * @version 4.0.0
 * @date 2010-05-02
 */

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <libgen.h>

#include "fcitx/fcitx.h"
#include "xdg.h"

static void make_path(const char *path);

void
make_path(const char *path)
{
    char opath[PATH_MAX];
    char *p;
    size_t len;

    strncpy(opath, path, sizeof(opath));
    opath[PATH_MAX - 1] = '\0';
    len = strlen(opath);

    while (opath[len - 1] == '/')
    {
        opath[len - 1] = '\0';
        len --;
    }

    for (p = opath; *p; p++)
        if (*p == '/')
        {
            *p = '\0';

            if (access(opath, F_OK))
                mkdir(opath, S_IRWXU);

            *p = '/';
        }

    if (access(opath, F_OK))        /* if path is not terminated with / */
        mkdir(opath, S_IRWXU);
}


FCITX_EXPORT_API
FILE *GetXDGFileWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char prefixpath[PATH_MAX];
    snprintf(prefixpath, PATH_MAX, "%s/%s", PACKAGE, prefix);
    char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", prefixpath , DATADIR, prefixpath);

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

FCITX_EXPORT_API
FILE *GetLibFile(const char *filename, const char *mode, char **retFile)
{
    size_t len;
    char ** path;
    path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/lib" , LIBDIR, PACKAGE);

    FILE* fp = GetXDGFile(filename, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;

}

FCITX_EXPORT_API
FILE *GetXDGFileUserWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char prefixpath[PATH_MAX];
    snprintf(prefixpath, PATH_MAX, "%s/%s", PACKAGE, prefix);
    char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", prefixpath , NULL, NULL);

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

FCITX_EXPORT_API
FILE *GetXDGFile(const char *fileName, char **path, const char *mode, size_t len, char **retFile)
{
    char buf[PATH_MAX];
    size_t i;
    FILE *fp = NULL;

    /* check absolute path */

    if (strlen(fileName) > 0 && fileName[0] == '/')
    {
        fp = fopen(fileName, mode);

        if (retFile)
            *retFile = strdup(fileName);

        return fp;
    }

    if (len <= 0)
        return NULL;

    if (!mode)
    {
        snprintf(buf, sizeof(buf), "%s/%s", path[0], fileName);
        buf[sizeof(buf) - 1] = '\0';

        if (retFile)
            *retFile = strdup(buf);

        return NULL;
    }

    for (i = 0;i < len;i++)
    {
        snprintf(buf, sizeof(buf), "%s/%s", path[i], fileName);
        buf[sizeof(buf) - 1] = '\0';

        fp = fopen(buf, mode);

        if (fp)
            break;

    }

    if (!fp)
    {
        if (strchr(mode, 'w') || strchr(mode, 'a'))
        {
            snprintf(buf, sizeof(buf), "%s/%s", path[0], fileName);
            buf[sizeof(buf) - 1] = '\0';

            char *dirc = strdup(buf);
            char *dir = dirname(dirc);
            make_path(dir);
            fp = fopen(buf, mode);
            free(dirc);
        }
    }

    if (retFile)
        *retFile = strdup(buf);

    return fp;
}

FCITX_EXPORT_API
void FreeXDGPath(char **path)
{
    free(path[0]);
    free(path);
}

FCITX_EXPORT_API
char **GetXDGPath(
    size_t *len,
    const char* homeEnv,
    const char* homeDefault,
    const char* suffixHome,
    const char* dirsDefault,
    const char* suffixGlobal)
{
    char* dirHome;
    const char *xdgDirHome = getenv(homeEnv);

    if (xdgDirHome && xdgDirHome[0])
    {
        dirHome = strdup(xdgDirHome);
    }
    else
    {
        const char *home = getenv("HOME");
        dirHome = malloc(strlen(home) + 1 + strlen(homeDefault) + 1);
        sprintf(dirHome, "%s/%s", home, homeDefault);
    }

    char *dirs;

    if (dirsDefault)
        asprintf(&dirs, "%s/%s:%s/%s", dirHome, suffixHome , dirsDefault, suffixGlobal);
    else
        asprintf(&dirs, "%s/%s", dirHome, suffixHome);

    free(dirHome);

    /* count dirs and change ':' to '\0' */
    size_t dirsCount = 1;

    char *tmp = dirs;

    while (*tmp)
    {
        if (*tmp == ':')
        {
            *tmp = '\0';
            dirsCount++;
        }

        tmp++;
    }

    /* alloc char pointer array and puts locations */
    size_t i;

    char **dirsArray = malloc(dirsCount * sizeof(char*));

    for (i = 0; i < dirsCount; ++i)
    {
        dirsArray[i] = dirs;

        while (*dirs)
        {
            dirs++;
        }

        dirs++;
    }

    *len = dirsCount;

    return dirsArray;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
