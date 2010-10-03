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
#include "core/fcitx.h"

#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <pthread.h>
#include <sys/stat.h>
#include <libgen.h>

#include "fcitx-config/xdg.h"
#include "fcitx-config/sprintf.h"

static void make_path (const char *path);

void
make_path (const char *path)
{
    char opath[PATH_MAX];
    char *p;
    size_t len;
    
    strncpy(opath, path, sizeof(opath));
    opath[PATH_MAX - 1] = '\0';
    len = strlen(opath);
    while(opath[len - 1] == '/')
    {
        opath[len - 1] = '\0';
        len --;
    }
    for(p = opath; *p; p++)
        if(*p == '/') {
            *p = '\0';
            if(access(opath, F_OK))
                mkdir(opath, S_IRWXU);
            *p = '/';
        }
    if(access(opath, F_OK))         /* if path is not terminated with / */
        mkdir(opath, S_IRWXU);
}


/** 
 * @brief 获得xdg路径的数据文件
 * 
 * @param 文件名
 * @param 模式
 * 
 * @return 文件指针
 */
FILE *GetXDGFileData(const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx" , DATADIR, "fcitx/data" );

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

FILE *GetLibFile(const char *filename, const char *mode, char **retFile)
{
    size_t len;
    char ** path;
    path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/lib" , LIBDIR, "" );

    FILE* fp = GetXDGFile(filename, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;

}

FILE *GetXDGFileTable(const char *fileName, const char *mode, char **retFile, Bool forceUser)
{
    size_t len;
    char ** path;
    if (forceUser)
        path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/table" , NULL, NULL );
    else
        path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/table" , DATADIR, "fcitx/data/table" );

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

FILE *GetXDGFilePinyin(const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char ** path;
    path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/pinyin" , DATADIR, "fcitx/data/pinyin" );

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

FILE *GetXDGFileUser(const char *fileName, const char *mode, char **retFile)
{
    size_t len;
    char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx" , NULL, NULL );

    FILE* fp = GetXDGFile(fileName, path, mode, len, retFile);

    FreeXDGPath(path);

    return fp;
}

/** 
 * @brief 根据Path按顺序查找第一个符合要求的文件，在mode包含写入时，这个函数会尝试创建文件夹
 * 
 * @param 文件名
 * @param 路径数组
 * @param 模式
 * @param 路径数值长度
 * 
 * @return 文件指针
 */
FILE *GetXDGFile(const char *fileName, char **path, const char *mode, size_t len, char **retFile)
{
    char buf[PATH_MAX];
    int i;
    FILE *fp = NULL;

    /* check absolute path */
    if (strlen(fileName) > 0 && fileName[0] == '/')
    {
        fp = fopen (fileName, mode);
        if (retFile)
            *retFile = strdup(fileName);
        return fp;
    }

    if (!mode)
    {
        snprintf(buf, sizeof(buf), "%s/%s", path[0], fileName);
        buf[sizeof(buf) - 1] = '\0';

        if (retFile)
            *retFile = strdup(buf);
        return NULL;
    }

    if (len <= 0)
        return NULL;
    for(i=0;i<len;i++)
    {
        snprintf(buf, sizeof(buf), "%s/%s", path[i], fileName);
        buf[sizeof(buf) - 1] = '\0';

        fp = fopen (buf, mode);
        if (fp)
            break;

    }

    if (!fp)
    {
        if (strchr(mode, 'w'))
        {
            snprintf(buf, sizeof(buf), "%s/%s", path[0], fileName);
            buf[sizeof(buf) - 1] = '\0';

            char *dirc = strdup(buf);
            char *dir = dirname(dirc);
            make_path(dir);
            fp = fopen (buf, mode);
            free(dirc);
        }
    }
    if (retFile)
        *retFile = strdup(buf);
    return fp;
}

/** 
 * @brief 释放路径数组的内存
 * 
 * @param 路径数组
 */
void FreeXDGPath(char **path)
{
    free(path[0]);
    free(path);
}

/** 
 * @brief 获得字符串数组，返回对应的XDGPath
 * 
 * @param 用于返回字符串数组长度的指针
 * @param XDG_*_HOME环境变量名称
 * @param 默认XDG_*_HOME路径
 * @param XDG_*_DIRS环境变量名称
 * @param 默认XDG_*_DIRS路径
 * 
 * @return 字符串数组
 */
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


