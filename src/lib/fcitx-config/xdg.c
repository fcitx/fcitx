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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
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
#include <dirent.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/utils.h"
#include "xdg.h"

static void make_path(const char *path);

void
make_path(const char *path)
{
    char *opath;
    char *p;
    size_t len;

    opath = strdup(path);
    len = strlen(opath);

    while (opath[len - 1] == '/') {
        opath[len - 1] = '\0';
        len --;
    }

    for (p = opath; *p; p++)
        if (*p == '/') {
            *p = '\0';

            if (access(opath, F_OK))
                mkdir(opath, S_IRWXU);

            *p = '/';
        }

    if (access(opath, F_OK))        /* if path is not terminated with / */
        mkdir(opath, S_IRWXU);

    free(opath);
}


FCITX_EXPORT_API
FILE *FcitxXDGGetFileWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char *prefixpath;
    asprintf(&prefixpath, "%s/%s", PACKAGE, prefix);
    char ** path = FcitxXDGGetPath(&len, "XDG_CONFIG_HOME", ".config", prefixpath , DATADIR, prefixpath);
    free(prefixpath);

    FILE* fp = FcitxXDGGetFile(fileName, path, mode, len, retFile);

    FcitxXDGFreePath(path);

    return fp;
}

FCITX_EXPORT_API
FILE *FcitxXDGGetLibFile(const char *filename, const char *mode, char **retFile)
{
    size_t len;
    char ** path;
    path = FcitxXDGGetPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/lib" , LIBDIR, PACKAGE);

    FILE* fp = FcitxXDGGetFile(filename, path, mode, len, retFile);

    FcitxXDGFreePath(path);

    return fp;

}

FCITX_EXPORT_API
FILE *FcitxXDGGetFileUserWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char *prefixpath;
    asprintf(&prefixpath, "%s/%s", PACKAGE, prefix);
    char ** path = FcitxXDGGetPath(&len, "XDG_CONFIG_HOME", ".config", prefixpath , NULL, NULL);
    free(prefixpath);

    FILE* fp = FcitxXDGGetFile(fileName, path, mode, len, retFile);

    FcitxXDGFreePath(path);

    return fp;
}

FCITX_EXPORT_API
FILE *FcitxXDGGetFile(const char *fileName, char **path, const char *mode, size_t len, char **retFile)
{
    char* buf = NULL;
    size_t i;
    FILE *fp = NULL;

    /* check absolute path */

    if (strlen(fileName) > 0 && fileName[0] == '/') {
        fp = fopen(fileName, mode);

        if (retFile)
            *retFile = strdup(fileName);

        return fp;
    }

    if (len <= 0)
        return NULL;

    if (!mode && retFile) {
        asprintf(retFile, "%s/%s", path[0], fileName);

        return NULL;
    }

    for (i = 0; i < len; i++) {
        asprintf(&buf, "%s/%s", path[i], fileName);

        fp = fopen(buf, mode);

        if (fp)
            break;

        free(buf);
        buf = NULL;
    }

    if (!fp) {
        if (strchr(mode, 'w') || strchr(mode, 'a')) {
            asprintf(&buf, "%s/%s", path[0], fileName);

            char *dirc = strdup(buf);
            char *dir = dirname(dirc);
            make_path(dir);
            fp = fopen(buf, mode);
            free(dirc);
        }
    }

    if (retFile)
        *retFile = buf;
    else if (buf)
        free(buf);

    return fp;
}

FCITX_EXPORT_API
void FcitxXDGFreePath(char **path)
{
    free(path[0]);
    free(path);
}

FCITX_EXPORT_API
char **FcitxXDGGetPath(
    size_t *len,
    const char* homeEnv,
    const char* homeDefault,
    const char* suffixHome,
    const char* dirsDefault,
    const char* suffixGlobal)
{
    char* dirHome;
    const char *xdgDirHome = getenv(homeEnv);

    if (xdgDirHome && xdgDirHome[0]) {
        dirHome = strdup(xdgDirHome);
    } else {
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

    while (*tmp) {
        if (*tmp == ':') {
            *tmp = '\0';
            dirsCount++;
        }

        tmp++;
    }

    /* alloc char pointer array and puts locations */
    size_t i;

    char **dirsArray = malloc(dirsCount * sizeof(char*));

    for (i = 0; i < dirsCount; ++i) {
        dirsArray[i] = dirs;

        while (*dirs) {
            dirs++;
        }

        dirs++;
    }

    *len = dirsCount;

    return dirsArray;
}

FCITX_EXPORT_API
FcitxStringHashSet* FcitxXDGGetFiles(
    char* path,
    char* prefix,
    char* suffix
)
{
    char **xdgPath;
    size_t len;
    char *pathBuf;
    size_t i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

    FcitxStringHashSet* sset = NULL;

    char *prefixpath;
    asprintf(&prefixpath, "%s/%s", PACKAGE, path);
    xdgPath = FcitxXDGGetPath(&len, "XDG_CONFIG_HOME", ".config" , prefixpath , DATADIR , prefixpath);
    free(prefixpath);

    for (i = 0; i < len; i++) {
        asprintf(&pathBuf, "%s", xdgPath[i]);

        dir = opendir(pathBuf);
        free(pathBuf);
        if (dir == NULL)
            continue;
        
        size_t suffixlen = 0;
        size_t prefixlen = 0;
        
        if (suffix) suffixlen = strlen(suffix);
        if (prefix) prefixlen = strlen(prefix);

        /* collect all *.conf files */
        while ((drt = readdir(dir)) != NULL) {
            size_t nameLen = strlen(drt->d_name);
            if (nameLen <= suffixlen + prefixlen)
                continue;

            if (suffix && strcmp(drt->d_name + nameLen - suffixlen, suffix) != 0)
                continue;
            if (prefix && strncmp(drt->d_name, prefix, prefixlen) != 0)
                continue;
            asprintf(&pathBuf, "%s/%s", xdgPath[i], drt->d_name);

            int statresult = stat(pathBuf, &fileStat);
            free(pathBuf);
            if (statresult == -1)
                continue;

            if (fileStat.st_mode & S_IFREG) {
                FcitxStringHashSet *string;
                HASH_FIND_STR(sset, drt->d_name, string);
                if (!string) {
                    char *bStr = strdup(drt->d_name);
                    string = malloc(sizeof(FcitxStringHashSet));
                    memset(string, 0, sizeof(FcitxStringHashSet));
                    string->name = bStr;
                    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
                }
            }
        }

        closedir(dir);
    }

    FcitxXDGFreePath(xdgPath);

    return sset;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
