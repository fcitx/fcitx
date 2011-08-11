/***************************************************************************
 *   Copyright (C) 2002~2005 by Yuking                                     *
 *   yuking_net@sohu.com                                                   *
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
 * @file   utils.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 * @brief  misc util function
 *
 */

#include <pthread.h>
#include <stdio.h>
#include <libintl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "fcitx/fcitx.h"
#include "utils.h"
#include "utf8.h"
#include <ctype.h>

FCITX_EXPORT_API
int CalculateRecordNumber (FILE * fpDict)
{
    char           *strBuf = NULL;
    size_t          bufLen = 0;
    int             nNumber = 0;

    while (getline(&strBuf, &bufLen, fpDict) != -1) {
        nNumber++;
    }
    rewind (fpDict);

    if (strBuf)
        free(strBuf);

    return nNumber;
}

FCITX_EXPORT_API
void *custom_bsearch(const void *key, const void *base,
                     size_t nmemb, size_t size, int accurate,
                     int (*compar)(const void *, const void *))
{
    if (accurate)
        return bsearch(key, base, nmemb, size, compar);
    else
    {
        size_t l, u, idx;
        const void *p;
        int comparison;

        l = 0;
        u = nmemb;
        while (l < u)
        {
            idx = (l + u) / 2;
            p = (void *) (((const char *) base) + (idx * size));
            comparison = (*compar) (key, p);
            if (comparison <= 0)
                u = idx;
            else if (comparison > 0)
                l = idx + 1;
        }

        if (u >= nmemb)
            return NULL;
        else
            return (void *) (((const char *) base) + (l * size));
    }
}

FCITX_EXPORT_API
void InitAsDaemon()
{
    pid_t pid;
    if ((pid = fork()) > 0)
    {
        waitpid(pid, NULL, 0);
        exit(0);
    }
    setsid();
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    if (fork() > 0)
        exit(0);
    chdir("/");

    umask(0);
}

FCITX_EXPORT_API
UT_array* SplitString(const char *str, char delm)
{
    UT_array* array;
    utarray_new(array, &ut_str_icd);
    char *bakstr = strdup(str);
    size_t len = strlen(bakstr);
    size_t i = 0, last = 0;
    for (i =0 ; i <= len ; i++)
    {
        if (bakstr[i] == delm || bakstr[i] == '\0')
        {
            bakstr[i] = '\0';
            char *p = &bakstr[last];
            if (strlen(p) > 0)
                utarray_push_back(array, &p);
            last = i + 1;
        }
    }
    free(bakstr);
    return array;
}

FCITX_EXPORT_API
void FreeStringList(UT_array *list)
{
    utarray_free(list);
}

FCITX_EXPORT_API
void *fcitx_malloc0(size_t bytes)
{
    void *p = malloc(bytes);
    if (!p)
        return NULL;

    memset(p, 0, bytes);
    return p;
}

FCITX_EXPORT_API
char *fcitx_trim(char *s)
{
    register char *end;
    register char csave;

    while (isspace(*s))                 /* skip leading space */
        ++s;
    end = strchr(s,'\0') - 1;
    while (end >= s && isspace(*end))               /* skip trailing space */
        --end;

    csave = end[1];
    end[1] = '\0';
    s = strdup(s);
    end[1] = csave;
    return (s);
}

FCITX_EXPORT_API
int FcitxGetDisplayNumber()
{
    int displayNumber = 0;
    char* display = getenv ("DISPLAY"), *strDisplayNumber = NULL;
    if (display != NULL)
    {
        display = strdup(display);
        char* p = display;
        for (; *p != ':' && *p != '\0'; p++);

        if (*p == ':') {
            *p = '\0';
            p++;
            strDisplayNumber = p;
        }
        for (; *p != '.' && *p != '\0'; p++);

        if (*p == '.') {
            *p = '\0';
        }

        sscanf(strDisplayNumber, "%d", &displayNumber);

        free(display);
    }
    return displayNumber;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
