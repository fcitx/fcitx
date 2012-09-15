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
 * xdg related path handle
 * @author CSSlayer
 * @version 4.0.0
 * @date 2010-05-02
 */

#define FCITX_CONFIG_XDG_DEPRECATED

#include "config.h"

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

static void
combine_path_with_len(char *dest, const char *str1, size_t len1,
                      const char *str2, size_t len2)
{
    memcpy(dest, str1, len1);
    dest += len1;
    dest[0] = '/';
    dest++;
    memcpy(dest, str2, len2);
    dest[len2] = '\0';
}

#define tmp_combine(dest, str1, str2)                                   \
    size_t __##dest##_len1 = strlen(str1);                              \
    size_t __##dest##_len2 = strlen(str2);                              \
    char dest[__##dest##_len1 + __##dest##_len2 + 2];                   \
    combine_path_with_len(dest, str1, __##dest##_len1, str2, __##dest##_len2);

static char*
acombine_path(const char *str1, const char *str2)
{
    size_t len1 = strlen(str1);
    size_t len2 = strlen(str2);
    char *dest = malloc(len1 + len2 + 2);
    combine_path_with_len(dest, str1, len1, str2, len2);
    return dest;
}

static void
make_path(const char *path)
{
    char *p;
    size_t len = strlen(path);
    char opath[len + 1];
    memcpy(opath, path, len + 1);

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
}


FCITX_EXPORT_API
FILE *FcitxXDGGetFileWithPrefix(const char *prefix, const char *fileName,
                                const char *mode, char **retFile)
{
    size_t len;
    char **path = FcitxXDGGetPathWithPrefix(&len, prefix);
    FILE *fp = FcitxXDGGetFile(fileName, path, mode, len, retFile);

    FcitxXDGFreePath(path);
    return fp;
}

FCITX_EXPORT_API
FILE *FcitxXDGGetLibFile(const char *filename, const char *mode, char **retFile)
{
    size_t len;
    char **path;
    char *libdir = fcitx_utils_get_fcitx_path("libdir");
    path = FcitxXDGGetPath(&len, "XDG_CONFIG_HOME", ".config",
                           PACKAGE "/lib" , libdir, PACKAGE);
    free(libdir);

    FILE* fp = FcitxXDGGetFile(filename, path, mode, len, retFile);
    FcitxXDGFreePath(path);
    return fp;
}

FCITX_EXPORT_API
FILE *FcitxXDGGetFileUserWithPrefix(const char* prefix, const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char ** path = FcitxXDGGetPathUserWithPrefix(&len, prefix);

    FILE* fp = FcitxXDGGetFile(fileName, path, mode, len, retFile);

    FcitxXDGFreePath(path);

    return fp;
}

FCITX_EXPORT_API
FILE *FcitxXDGGetFile(const char *fileName, char **path, const char *mode,
                      size_t len, char **retFile)
{
    size_t i;
    FILE *fp = NULL;

    /* check absolute path */

    if (fileName[0] == '/') {
        if (mode)
            fp = fopen(fileName, mode);

        if (retFile)
            *retFile = strdup(fileName);

        return fp;
    }

    if (len <= 0)
        return NULL;

    if (!mode && retFile) {
        *retFile = acombine_path(path[0], fileName);
        return NULL;
    }

    if (!fileName[0]) {
        if (retFile) {
            *retFile = strdup(path[0]);
        }
        if (strchr(mode, 'w') || strchr(mode, 'a')) {
            make_path(path[0]);
        }
        return NULL;
    }

    char* buf = NULL;
    for (i = 0; i < len; i++) {
        fcitx_utils_free(buf);
        buf = acombine_path(path[i], fileName);
        fp = fopen(buf, mode);
        if (fp)
            break;
    }

    if (!fp) {
        if (strchr(mode, 'w') || strchr(mode, 'a')) {
            fcitx_utils_free(buf);
            buf = acombine_path(path[0], fileName);
            char *dirc = strdup(buf);
            char *dir = dirname(dirc);
            make_path(dir);
            fp = fopen(buf, mode);
            free(dirc);
        }
    }

    if (retFile) {
        *retFile = buf;
    } else if (buf) {
        free(buf);
    }
    return fp;
}

FCITX_EXPORT_API
void FcitxXDGFreePath(char **path)
{
    free(path[0]);
    free(path);
}

FCITX_EXPORT_API char**
FcitxXDGGetPath(size_t *len, const char* homeEnv, const char* homeDefault,
                const char* suffixHome, const char* dirsDefault,
                const char* suffixGlobal)
{
    const char *dirHome;
    const char *xdgDirHome = getenv(homeEnv);
    size_t dh_len, he_len, hd_len;
    const char *env_home;

    if (xdgDirHome && xdgDirHome[0]) {
        dirHome = xdgDirHome;
        dh_len = 0;
    } else {
        env_home = getenv("HOME");
        if (!(env_home && env_home[0]))
            return NULL;
        he_len = strlen(env_home);
        hd_len = strlen(homeDefault);
        dh_len = he_len + hd_len + 1;
    }

    char home_buff[dh_len + 1];
    if (dh_len) {
        dirHome = home_buff;
        combine_path_with_len(home_buff, env_home, he_len, homeDefault, hd_len);
    } else {
        dh_len = strlen(dirHome);
    }

    char *dirs;
    char **dirsArray;
    size_t sh_len = strlen(suffixHome);
    size_t orig_len1 = dh_len + sh_len;
    if (dirsDefault) {
        size_t dd_len = strlen(dirsDefault);
        size_t sg_len = strlen(suffixGlobal);
        *len = 2;
        dirs = malloc(orig_len1 + dd_len + sg_len + 4);
        dirsArray = malloc(2 * sizeof(char*));
        dirsArray[0] = dirs;
        dirsArray[1] = dirs + orig_len1 + 2;
        combine_path_with_len(dirsArray[0], dirHome, dh_len, suffixHome, sh_len);
        combine_path_with_len(dirsArray[1], dirsDefault, dd_len,
                              suffixGlobal, sg_len);
    } else {
        *len = 1;
        dirs = malloc(orig_len1 + 2);
        dirsArray = malloc(sizeof(char*));
        dirsArray[0] = dirs;
        combine_path_with_len(dirsArray[0], dirHome, dh_len, suffixHome, sh_len);
    }
    return dirsArray;
}

FCITX_EXPORT_API
char** FcitxXDGGetPathUserWithPrefix(size_t* len, const char* prefix)
{
    tmp_combine(prefixpath, PACKAGE, prefix);
    return FcitxXDGGetPath(len, "XDG_CONFIG_HOME", ".config",
                           prefixpath, NULL, NULL);
}

FCITX_EXPORT_API
char** FcitxXDGGetPathWithPrefix(size_t* len, const char* prefix)
{
    tmp_combine(prefixpath, PACKAGE, prefix);
    char *datadir = fcitx_utils_get_fcitx_path("datadir");
    char **xdgPath = FcitxXDGGetPath(len, "XDG_CONFIG_HOME", ".config",
                                     prefixpath, datadir, prefixpath);
    free(datadir);
    return xdgPath;
}


FCITX_EXPORT_API
FcitxStringHashSet* FcitxXDGGetFiles(char *path, char *prefix, char *suffix)
{
    char **xdgPath;
    size_t len;
    size_t i = 0;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;

    FcitxStringHashSet* sset = NULL;

    xdgPath = FcitxXDGGetPathWithPrefix(&len, path);

    for (i = 0; i < len; i++) {
        dir = opendir(xdgPath[i]);
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
            size_t len1 = strlen(xdgPath[i]);
            char path_buf[nameLen + len1 + 2];
            combine_path_with_len(path_buf, xdgPath[i], len1,
                                  drt->d_name, nameLen);
            int statresult = stat(path_buf, &fileStat);
            if (statresult == -1)
                continue;

            if (fileStat.st_mode & S_IFREG) {
                FcitxStringHashSet *string;
                HASH_FIND_STR(sset, drt->d_name, string);
                if (!string) {
                    char *bStr = strdup(drt->d_name);
                    string = fcitx_utils_new(FcitxStringHashSet);
                    string->name = bStr;
                    HASH_ADD_KEYPTR(hh, sset, string->name,
                                    strlen(string->name), string);
                }
            }
        }

        closedir(dir);
    }

    FcitxXDGFreePath(xdgPath);

    return sset;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
