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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
/**
 * @file   utils.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 *
 *  misc util function
 *
 */

#define FCITX_USE_INTERNAL_PATH

#include "config.h"

#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>
#include <stdarg.h>

#if defined(ENABLE_BACKTRACE)
#include <execinfo.h>
#endif

#include "fcitx/fcitx.h"
#include "utils.h"
#include "utf8.h"
#include "log.h"

#if defined(LIBKVM_FOUND)
#include <kvm.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#endif

#if defined(__linux__)
#include <sys/prctl.h>
#endif

FCITX_EXPORT_API
int fcitx_utils_calculate_record_number(FILE* fpDict)
{
    char           *strBuf = NULL;
    size_t          bufLen = 0;
    int             nNumber = 0;

    while (getline(&strBuf, &bufLen, fpDict) != -1) {
        nNumber++;
    }
    rewind(fpDict);

    if (strBuf)
        free(strBuf);

    return nNumber;
}

FCITX_EXPORT_API
void *fcitx_utils_custom_bsearch(const void *key, const void *base,
                                 size_t nmemb, size_t size, int accurate,
                                 int (*compar)(const void *, const void *))
{
    if (accurate)
        return bsearch(key, base, nmemb, size, compar);
    else {
        size_t l, u, idx;
        const void *p;
        int comparison;

        l = 0;
        u = nmemb;
        while (l < u) {
            idx = (l + u) / 2;
            p = (void *)(((const char *) base) + (idx * size));
            comparison = (*compar)(key, p);
            if (comparison <= 0)
                u = idx;
            else if (comparison > 0)
                l = idx + 1;
        }

        if (u >= nmemb)
            return NULL;
        else
            return (void *)(((const char *) base) + (l * size));
    }
}

typedef void (*_fcitx_sighandler_t) (int);

FCITX_EXPORT_API
void fcitx_utils_init_as_daemon()
{
    pid_t pid;
    if ((pid = fork()) > 0) {
        waitpid(pid, NULL, 0);
        exit(0);
    }
    setsid();
    _fcitx_sighandler_t oldint = signal(SIGINT, SIG_IGN);
    _fcitx_sighandler_t oldhup  =signal(SIGHUP, SIG_IGN);
    _fcitx_sighandler_t oldquit = signal(SIGQUIT, SIG_IGN);
    _fcitx_sighandler_t oldpipe = signal(SIGPIPE, SIG_IGN);
    _fcitx_sighandler_t oldttou = signal(SIGTTOU, SIG_IGN);
    _fcitx_sighandler_t oldttin = signal(SIGTTIN, SIG_IGN);
    _fcitx_sighandler_t oldchld = signal(SIGCHLD, SIG_IGN);
    if (fork() > 0)
        exit(0);
    chdir("/");

    signal(SIGINT, oldint);
    signal(SIGHUP, oldhup);
    signal(SIGQUIT, oldquit);
    signal(SIGPIPE, oldpipe);
    signal(SIGTTOU, oldttou);
    signal(SIGTTIN, oldttin);
    signal(SIGCHLD, oldchld);
}

FCITX_EXPORT_API
UT_array* fcitx_utils_new_string_list()
{
    UT_array* array;
    utarray_new(array, &ut_str_icd);
    return array;
}

FCITX_EXPORT_API
UT_array* fcitx_utils_split_string(const char* str, char delm)
{
    UT_array* array;
    utarray_new(array, &ut_str_icd);
    char *bakstr = strdup(str);
    size_t len = strlen(bakstr);
    size_t i = 0, last = 0;
    if (len) {
        for (i = 0 ; i <= len ; i++) {
            if (bakstr[i] == delm || bakstr[i] == '\0') {
                bakstr[i] = '\0';
                char *p = &bakstr[last];
                utarray_push_back(array, &p);
                last = i + 1;
            }
        }
    }
    free(bakstr);
    return array;
}

FCITX_EXPORT_API
void fcitx_utils_string_list_printf_append(UT_array* list, const char* fmt,...)
{
    char* buffer;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&buffer, fmt, ap);
    va_end(ap);
    utarray_push_back(list, &buffer);
    free(buffer);
}

FCITX_EXPORT_API
char* fcitx_utils_join_string_list(UT_array* list, char delm)
{
    if (!list)
        return NULL;

    if (utarray_len(list) == 0)
        return strdup("");

    size_t len = 0;
    char** str;
    for (str = (char**) utarray_front(list);
         str != NULL;
         str = (char**) utarray_next(list, str))
    {
        len += strlen(*str) + 1;
    }

    char* result = (char*) fcitx_utils_malloc0(sizeof(char) * len);
    char* p = result;
    for (str = (char**) utarray_front(list);
         str != NULL;
         str = (char**) utarray_next(list, str))
    {
        size_t strl = strlen(*str);
        strcpy(p, *str);
        p[strl] = delm;
        p += strl + 1;
    }
    result[len - 1] = '\0';

    return result;
}

FCITX_EXPORT_API
int fcitx_utils_string_list_contains(UT_array* list, const char* scmp)
{
    char** str;
    for (str = (char**) utarray_front(list);
         str != NULL;
         str = (char**) utarray_next(list, str))
    {
        if (strcmp(scmp, *str) == 0)
            return 1;
    }
    return 0;
}

FCITX_EXPORT_API
void fcitx_utils_free_string_list(UT_array* list)
{
    utarray_free(list);
}

FCITX_EXPORT_API
void fcitx_utils_free_string_hash_set(FcitxStringHashSet* sset)
{
    FcitxStringHashSet *curStr;
    while (sset) {
        curStr = sset;
        HASH_DEL(sset, curStr);
        free(curStr->name);
        free(curStr);
    }
}

FCITX_EXPORT_API
void* fcitx_utils_malloc0(size_t bytes)
{
    void *p = malloc(bytes);
    if (!p)
        return NULL;

    memset(p, 0, bytes);
    return p;
}

FCITX_EXPORT_API
char* fcitx_utils_trim(const char* s)
{
    register const char *end;

    while (isspace(*s))                 /* skip leading space */
        ++s;
    end = s + (strlen(s) - 1);
    while (end >= s && isspace(*end))               /* skip trailing space */
        --end;

    end++;

    size_t len = end - s;

    char* result = fcitx_utils_malloc0(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

FCITX_EXPORT_API
int fcitx_utils_get_display_number()
{
    int displayNumber = 0;
    char* display = getenv("DISPLAY"), *strDisplayNumber = NULL;
    if (display != NULL) {
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

        if (strDisplayNumber) {
            displayNumber = atoi(strDisplayNumber);
        }

        free(display);
    }
    return displayNumber;
}

FCITX_EXPORT_API
int fcitx_utils_current_locale_is_utf8()
{
    const char* p;
    p = getenv("LC_CTYPE");
    if (!p) {
        p = getenv("LC_ALL");
        if (!p)
            p = getenv("LANG");
    }
    if (p) {
        if (strcasestr(p, "utf8") || strcasestr(p, "utf-8"))
            return 1;
    }
    return 0;
}

FCITX_EXPORT_API
char* fcitx_utils_get_current_langcode()
{
    /* language[_territory][.codeset][@modifier]" or "C" */
    const char* p;
    p = getenv("LC_CTYPE");
    if (!p) {
        p = getenv("LC_ALL");
        if (!p)
            p = getenv("LANG");
    }
    if (p) {
        char* result = strdup(p);
        char* m;
        m = strchr(result, '.');
        if (m) *m = '\0';

        m = strchr(result, '@');
        if (m) *m= '\0';
        return result;
    }
    return strdup("C");
}

FCITX_EXPORT_API
int fcitx_utils_pid_exists(pid_t pid)
{
    if (pid <= 0)
        return 0;
    return !kill(pid, 0);
}

FCITX_EXPORT_API
char* fcitx_utils_get_process_name()
{
#if defined(__linux__)
    #define _PR_GET_NAME_MAX 16
    char name[_PR_GET_NAME_MAX + 1];
    if (prctl(PR_GET_NAME, (unsigned long)name, 0, 0, 0))
        return strdup("");
    name[_PR_GET_NAME_MAX] = '\0';
    return strdup(name);
    /**
     * Keep the old code here in case we want to get the name of
     * another process with known pid sometime.
     **/
    /* do { */
    /*     FILE* fp = fopen("/proc/self/stat", "r"); */
    /*     if (!fp) */
    /*         break; */

    /*     const size_t bufsize = 1024; */
    /*     char buf[bufsize]; */
    /*     fgets(buf, bufsize, fp); */
    /*     fclose(fp); */

    /*     char* S = strchr(buf, '('); */
    /*     if (!S) */
    /*         break; */
    /*     char* E = strchr(S, ')'); */
    /*     if (!E) */
    /*         break; */

    /*     return strndup(S+1, E-S-1); */
    /* } while(0); */
    /* return strdup(""); */
#elif defined(LIBKVM_FOUND)
    kvm_t *vm = kvm_open(0, "/dev/null", 0, O_RDONLY, NULL);
    if (vm == 0)
        return strdup("");

    char* result = NULL;
    do {
        int cnt;
        int mypid = getpid();
        struct kinfo_proc * kp = kvm_getprocs(vm, KERN_PROC_PID, mypid, &cnt);
        if ((cnt != 1) || (kp == 0)) {
            break;
        }
        int i;
        for (i = 0; i < cnt; i++)
            if (kp->ki_pid == mypid)
                break;
        if (i != cnt)
            result = strdup(kp->ki_comm);
    } while (0);
    kvm_close(vm);
    if (result == NULL)
        result = strdup("");
    return result;
#else
    return strdup("");
#endif
}

FCITX_EXPORT_API
char* fcitx_utils_get_fcitx_path(const char* type)
{
    char* fcitxdir = getenv("FCITXDIR");
    char* result = NULL;
    if (strcmp(type, "datadir") == 0) {
        if (fcitxdir) {
            asprintf(&result, "%s/share", fcitxdir);
        }
        else
            result = strdup(DATADIR);
    }
    else if (strcmp(type, "pkgdatadir") == 0) {
        if (fcitxdir) {
            asprintf(&result, "%s/share/" PACKAGE, fcitxdir);
        }
        else
            result = strdup(PKGDATADIR);
    }
    else if (strcmp(type, "bindir") == 0) {
        if (fcitxdir) {
            asprintf(&result, "%s/bin", fcitxdir);
        }
        else
            result = strdup(BINDIR);
    }
    else if (strcmp(type, "libdir") == 0) {
        if (fcitxdir) {
            asprintf(&result, "%s/lib", fcitxdir);
        }
        else
            result = strdup(LIBDIR);
    }
    else if (strcmp(type, "localedir") == 0) {
        if (fcitxdir) {
            asprintf(&result, "%s/share/locale", fcitxdir);
        }
        else
            result = strdup(LOCALEDIR);
    }
    return result;
}

FCITX_EXPORT_API
char* fcitx_utils_get_fcitx_path_with_filename(const char* type, const char* filename)
{
    char* path = fcitx_utils_get_fcitx_path(type);
    if (path == NULL)
        return NULL;
    char* result;
    asprintf(&result, "%s/%s", path, filename);
    free(path);
    return result;
}

FCITX_EXPORT_API
void fcitx_utils_string_swap(char** obj, const char* str)
{
    fcitx_utils_free(*obj);

    if (str)
        *obj = strdup(str);
    else
        *obj = NULL;
}

FCITX_EXPORT_API
void fcitx_utils_launch_tool(const char* name, const char* arg)
{
    char* command = fcitx_utils_get_fcitx_path_with_filename("bindir", name);
    char* darg = NULL;
    if (arg)
        darg = strdup(arg);
    char* args[] = {
        command,
        darg,
        0
    };
    fcitx_utils_start_process(args);
    fcitx_utils_free(darg);
    free(command);
}

FCITX_EXPORT_API
void fcitx_utils_launch_configure_tool()
{
    fcitx_utils_launch_tool("fcitx-configtool", NULL);
}

FCITX_EXPORT_API
void fcitx_utils_launch_configure_tool_for_addon(const char* imaddon)
{
    fcitx_utils_launch_tool("fcitx-configtool", imaddon);
}

FCITX_EXPORT_API
void fcitx_utils_launch_restart()
{
    fcitx_utils_launch_tool("fcitx", "-r");
}

FCITX_EXPORT_API
void fcitx_utils_start_process(char** args)
{
    /* exec command */
    pid_t child_pid;

    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
    } else if (child_pid == 0) {         /* child process  */
        pid_t grandchild_pid;

        grandchild_pid = fork();
        if (grandchild_pid < 0) {
            perror("fork");
            _exit(1);
        } else if (grandchild_pid == 0) { /* grandchild process  */
            execvp(args[0], args);
            perror("execvp");
            _exit(1);
        } else {
            _exit(0);
        }
    } else {                              /* parent process */
        int status;
        waitpid(child_pid, &status, 0);
    }
}

FCITX_EXPORT_API
void
fcitx_utils_backtrace()
{
#if defined(ENABLE_BACKTRACE)
    void *array[20];

    size_t size;
    char **strings = NULL;
    size_t i;

    size = backtrace(array, 20);
    strings = backtrace_symbols(array, size);

    if (strings) {
        fprintf(stderr, "Obtained %zd stack frames.\n", size);

        for (i = 0; i < size; i++) {
            fprintf(stderr, "%s\n", strings[i]);
        }

        free(strings);
    }
#endif
}

FCITX_EXPORT_API
int
fcitx_utils_get_boolean_env(const char *name,
                            int defval)
{
    const char *value = getenv(name);

    if (value == NULL)
        return defval;

    if (strcmp(value, "") == 0 ||
        strcmp(value, "0") == 0 ||
        strcmp(value, "false") == 0 ||
        strcmp(value, "False") == 0 ||
        strcmp(value, "FALSE") == 0)
        return 0;

    return 1;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
