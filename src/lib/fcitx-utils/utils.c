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
#include <errno.h>

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

#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#else
#include <sys/endian.h>
#endif

static const UT_icd __fcitx_ptr_icd = {
    sizeof(void*), NULL, NULL, NULL
};

FCITX_EXPORT_API const UT_icd *const fcitx_ptr_icd = &__fcitx_ptr_icd;
FCITX_EXPORT_API const UT_icd *const fcitx_str_icd = &ut_str_icd;
FCITX_EXPORT_API const UT_icd *const fcitx_int_icd = &ut_int_icd;

FCITX_EXPORT_API UT_array*
fcitx_utils_string_list_append_no_copy(UT_array *list, char *str)
{
    utarray_extend_back(list);
    *(char**)utarray_back(list) = str;
    return list;
}

FCITX_EXPORT_API UT_array*
fcitx_utils_string_list_append_len(UT_array *list, const char *str, size_t len)
{
    char *buff = fcitx_utils_set_str_with_len(NULL, str, len);
    fcitx_utils_string_list_append_no_copy(list, buff);
    return list;
}

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

    fcitx_utils_free(strBuf);

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

FCITX_EXPORT_API UT_array*
fcitx_utils_new_string_list()
{
    UT_array *array;
    utarray_new(array, fcitx_str_icd);
    return array;
}

FCITX_EXPORT_API UT_array*
fcitx_utils_append_split_string(UT_array *list,
                                const char* str, const char *delm)
{
    const char *src = str;
    const char *pos;
    size_t len;
    while ((len = strcspn(src, delm)), *(pos = src + len)) {
        fcitx_utils_string_list_append_len(list, src, len);
        src = pos + 1;
    }
    if (len)
        fcitx_utils_string_list_append_len(list, src, len);
    return list;
}

FCITX_EXPORT_API
UT_array* fcitx_utils_split_string(const char* str, char delm)
{
    UT_array* array;
    char delm_s[2] = {delm, '\0'};
    utarray_new(array, fcitx_str_icd);
    return fcitx_utils_append_split_string(array, str, delm_s);
}

FCITX_EXPORT_API
void fcitx_utils_string_list_printf_append(UT_array* list, const char* fmt,...)
{
    char* buffer;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&buffer, fmt, ap);
    va_end(ap);
    fcitx_utils_string_list_append_no_copy(list, buffer);
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

    char* result = (char*)malloc(sizeof(char) * len);
    char* p = result;
    for (str = (char**) utarray_front(list);
         str != NULL;
         str = (char**) utarray_next(list, str))
    {
        size_t strl = strlen(*str);
        memcpy(p, *str, strl);
        p += strl;
        *p = delm;
        p++;
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
char* fcitx_utils_string_hash_set_join(FcitxStringHashSet* sset, char delim)
{
    if (!sset)
        return NULL;

    if (HASH_COUNT(sset) == 0)
        return strdup("");

    size_t len = 0;
    HASH_FOREACH(string, sset, FcitxStringHashSet) {
        len += strlen(string->name) + 1;
    }

    char* result = (char*)malloc(sizeof(char) * len);
    char* p = result;
    HASH_FOREACH(string2, sset, FcitxStringHashSet) {
        size_t strl = strlen(string2->name);
        memcpy(p, string2->name, strl);
        p += strl;
        *p = delim;
        p++;
    }
    result[len - 1] = '\0';

    return result;
}

FCITX_EXPORT_API
FcitxStringHashSet* fcitx_utils_string_hash_set_parse(const char* str, char delim)
{
    FcitxStringHashSet* sset = NULL;
    const char *src = str;
    const char *pos;
    size_t len;
    
    char delim_s[2] = {delim, '\0'};
    while ((len = strcspn(src, delim_s)), *(pos = src + len)) {
        sset = fcitx_utils_string_hash_set_insert_len(sset, src, len);
        src = pos + 1;
    }
    if (len)
        sset = fcitx_utils_string_hash_set_insert_len(sset, src, len);
    return sset;
}

FCITX_EXPORT_API
FcitxStringHashSet* fcitx_utils_string_hash_set_insert(FcitxStringHashSet* sset, const char* str)
{
    FcitxStringHashSet* string = fcitx_utils_new(FcitxStringHashSet);
    string->name = strdup(str);
    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
    return sset;
}

FCITX_EXPORT_API
FcitxStringHashSet* fcitx_utils_string_hash_set_insert_len(FcitxStringHashSet* sset, const char* str, size_t len)
{
    FcitxStringHashSet* string = fcitx_utils_new(FcitxStringHashSet);
    string->name = strndup(str, len);
    HASH_ADD_KEYPTR(hh, sset, string->name, strlen(string->name), string);
    return sset;
}

FCITX_EXPORT_API
boolean fcitx_utils_string_hash_set_contains(FcitxStringHashSet* sset, const char* str)
{
    FcitxStringHashSet* string = NULL;
    HASH_FIND_STR(sset, str, string);
    return (string != NULL);
}

FCITX_EXPORT_API
FcitxStringHashSet* fcitx_util_string_hash_set_remove(FcitxStringHashSet* sset, const char* str)
{
    FcitxStringHashSet* string = NULL;
    HASH_FIND_STR(sset, str, string);
    if (string) {
        HASH_DEL(sset, string);
        free(string->name);
        free(string);
    }
    return sset;
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

    s += strspn(s, "\f\n\r\t\v ");
    end = s + (strlen(s) - 1);
    while (end >= s && isspace(*end))               /* skip trailing space */
        --end;

    end++;

    size_t len = end - s;

    char* result = malloc(len + 1);
    memcpy(result, s, len);
    result[len] = '\0';
    return result;
}

FCITX_EXPORT_API int
fcitx_utils_get_display_number()
{
    const char *display = getenv("DISPLAY");
    if (!display)
        return 0;
    size_t len;
    const char *p = display + strcspn(display, ":");
    if (*p != ':')
        return 0;
    p++;
    len = strcspn(p, ".");
    char *str_disp_num = fcitx_utils_set_str_with_len(NULL, p, len);
    int displayNumber = atoi(str_disp_num);
    free(str_disp_num);
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
    if (p)
        return fcitx_utils_set_str_with_len(NULL, p, strcspn(p, ".@"));
    return strdup("C");
}

FCITX_EXPORT_API
int fcitx_utils_pid_exists(pid_t pid)
{
    if (pid <= 0)
        return 0;
    return !(kill(pid, 0) && (errno == ESRCH));
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
            fcitx_utils_alloc_cat_str(result, fcitxdir, "/share");
        } else {
            result = strdup(DATADIR);
        }
    }
    else if (strcmp(type, "pkgdatadir") == 0) {
        if (fcitxdir) {
            fcitx_utils_alloc_cat_str(result, fcitxdir, "/share/"PACKAGE);
        } else {
            result = strdup(PKGDATADIR);
        }
    }
    else if (strcmp(type, "bindir") == 0) {
        if (fcitxdir) {
            fcitx_utils_alloc_cat_str(result, fcitxdir, "/bin");
        }
        else
            result = strdup(BINDIR);
    }
    else if (strcmp(type, "libdir") == 0) {
        if (fcitxdir) {
            fcitx_utils_alloc_cat_str(result, fcitxdir, "/lib");
        }
        else
            result = strdup(LIBDIR);
    }
    else if (strcmp(type, "localedir") == 0) {
        if (fcitxdir) {
            fcitx_utils_alloc_cat_str(result, fcitxdir, "/share/locale");
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
    char *result;
    fcitx_utils_alloc_cat_str(result, path, "/", filename);
    free(path);
    return result;
}

FCITX_EXPORT_API
void fcitx_utils_string_swap(char** obj, const char* str)
{
    if (str) {
        *obj = fcitx_utils_set_str(*obj, str);
    } else if (*obj) {
        free(*obj);
        *obj = NULL;
    }
}

FCITX_EXPORT_API
void fcitx_utils_string_swap_with_len(char** obj, const char* str, size_t len)
{
    if (str) {
        *obj = fcitx_utils_set_str_with_len(*obj, str, len);
    } else if (*obj) {
        free(*obj);
        *obj = NULL;
    }
}

FCITX_EXPORT_API void
fcitx_utils_launch_tool(const char* name, const char* arg)
{
    char* command = fcitx_utils_get_fcitx_path_with_filename("bindir", name);
    char* args[] = {
        command,
        (char*)(intptr_t)arg, /* parent process haven't even touched this... */
        NULL
    };
    fcitx_utils_start_process(args);
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
    void *array[32];

    size_t size;

    size = backtrace(array, 32);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
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

    if ((!*value) ||
        strcmp(value, "0") == 0 ||
        strcasecmp(value, "false") == 0)
        return 0;

    return 1;
}

FCITX_EXPORT_API
size_t
fcitx_utils_read_uint32(FILE *fp, uint32_t *p)
{
    uint32_t res = 0;
    size_t size;
    size = fread(&res, sizeof(uint32_t), 1, fp);
    *p = le32toh(res);
    return size;
}

FCITX_EXPORT_API
size_t
fcitx_utils_write_uint32(FILE *fp, uint32_t i)
{
    i = htole32(i);
    return fwrite(&i, sizeof(uint32_t), 1, fp);
}

FCITX_EXPORT_API
size_t
fcitx_utils_read_uint16(FILE *fp, uint16_t *p)
{
    uint16_t res = 0;
    size_t size;
    size = fread(&res, sizeof(uint16_t), 1, fp);
    *p = le16toh(res);
    return size;
}

FCITX_EXPORT_API
size_t
fcitx_utils_write_uint16(FILE *fp, uint16_t i)
{
    i = htole16(i);
    return fwrite(&i, sizeof(uint16_t), 1, fp);
}

FCITX_EXPORT_API
size_t
fcitx_utils_read_uint64(FILE *fp, uint64_t *p)
{
    uint64_t res = 0;
    size_t size;
    size = fread(&res, sizeof(uint64_t), 1, fp);
    *p = le64toh(res);
    return size;
}

FCITX_EXPORT_API
size_t
fcitx_utils_write_uint64(FILE *fp, uint64_t i)
{
    i = htole64(i);
    return fwrite(&i, sizeof(uint64_t), 1, fp);
}

FCITX_EXPORT_API size_t
fcitx_utils_str_lens(size_t n, const char **str_list, size_t *size_list)
{
    size_t i;
    size_t total = 0;
    for (i = 0;i < n;i++) {
        total += (size_list[i] = str_list[i] ? strlen(str_list[i]) : 0);
    }
    return total + 1;
}

FCITX_EXPORT_API void
fcitx_utils_cat_str(char *out, size_t n, const char **str_list,
                        const size_t *size_list)
{
    size_t i = 0;
    for (i = 0;i < n;i++) {
        if (!size_list[i])
            continue;
        memcpy(out, str_list[i], size_list[i]);
        out += size_list[i];
    }
    *out = '\0';
}

FCITX_EXPORT_API void
fcitx_utils_cat_str_with_len(char *out, size_t len, size_t n,
                             const char **str_list, const size_t *size_list)
{
    char *limit = out + len - 1;
    char *tmp = out;
    size_t i = 0;
    for (i = 0;i < n;i++) {
        if (!size_list[i])
            continue;
        tmp += size_list[i];
        if (tmp > limit) {
            memcpy(out, str_list[i], limit - out);
            out = limit;
            break;
        }
        memcpy(out, str_list[i], size_list[i]);
        out = tmp;
    }
    *out = '\0';
}

FCITX_EXPORT_API
int
fcitx_utils_strcmp0(const char* a, const char* b)
{
    if (a == NULL && b == NULL)
        return 0;
    if (a == NULL && b)
        return -1;
    if (a && b == NULL)
        return 1;
    return strcmp(a, b);

}

FCITX_EXPORT_API
int
fcitx_utils_strcmp_empty(const char* a, const char* b)
{
    int isemptya = (a == NULL || (*a) == 0);
    int isemptyb = (b == NULL || (*b) == 0);
    if (isemptya && isemptyb)
        return 0;
    if (isemptya && !isemptyb)
        return -1;
    if (!isemptya && isemptyb)
        return 1;
    return strcmp(a, b);
}

FCITX_EXPORT_API char*
fcitx_utils_set_str_with_len(char *res, const char *str, size_t len)
{
    if (res) {
        res = realloc(res, len + 1);
    } else {
        res = malloc(len + 1);
    }
    memcpy(res, str, len);
    res[len] = '\0';
    return res;
}

FCITX_EXPORT_API char
fcitx_utils_unescape_char(char c)
{
    switch (c) {
#define CASE_UNESCAPE(from, to) case from: return to
        CASE_UNESCAPE('a', '\a');
        CASE_UNESCAPE('b', '\b');
        CASE_UNESCAPE('f', '\f');
        CASE_UNESCAPE('n', '\n');
        CASE_UNESCAPE('r', '\r');
        CASE_UNESCAPE('t', '\t');
        CASE_UNESCAPE('e', '\e');
        CASE_UNESCAPE('v', '\v');
#undef CASE_UNESCAPE
    }
    return c;
}

FCITX_EXPORT_API char*
fcitx_utils_unescape_str_inplace(char *str)
{
    char *dest = str;
    char *src = str;
    char *pos;
    size_t len;
    while ((len = strcspn(src, "\\")), *(pos = src + len)) {
        if (dest != src && len)
            memmove(dest, src, len);
        dest += len;
        src = pos + 1;
        *dest = fcitx_utils_unescape_char(*src);
        dest++;
        src++;
    }
    if (dest != src && len)
        memmove(dest, src, len);
    dest[len] = '\0';
    return str;
}

FCITX_EXPORT_API char*
fcitx_utils_set_unescape_str(char *res, const char *str)
{
    size_t len = strlen(str) + 1;
    if (res) {
        res = realloc(res, len);
    } else {
        res = malloc(len);
    }
    char *dest = res;
    const char *src = str;
    const char *pos;
    while ((len = strcspn(src, "\\")), *(pos = src + len)) {
        memcpy(dest, src, len);
        dest += len;
        src = pos + 1;
        *dest = fcitx_utils_unescape_char(*src);
        dest++;
        src++;
    }
    if (len)
        memcpy(dest, src, len);
    dest[len] = '\0';
    return res;
}

FCITX_EXPORT_API char
fcitx_utils_escape_char(char c)
{
    switch (c) {
#define CASE_ESCAPE(to, from) case from: return to
        CASE_ESCAPE('a', '\a');
        CASE_ESCAPE('b', '\b');
        CASE_ESCAPE('f', '\f');
        CASE_ESCAPE('n', '\n');
        CASE_ESCAPE('r', '\r');
        CASE_ESCAPE('t', '\t');
        CASE_ESCAPE('e', '\e');
        CASE_ESCAPE('v', '\v');
#undef CASE_ESCAPE
    }
    return c;
}

FCITX_EXPORT_API char*
fcitx_utils_set_escape_str_with_set(char *res, const char *str, const char *set)
{
    if (!set)
        set = FCITX_CHAR_NEED_ESCAPE;
    size_t len = strlen(str) * 2 + 1;
    if (res) {
        res = realloc(res, len);
    } else {
        res = malloc(len);
    }
    char *dest = res;
    const char *src = str;
    const char *pos;
    while ((len = strcspn(src, set)), *(pos = src + len)) {
        memcpy(dest, src, len);
        dest += len;
        *dest = '\\';
        dest++;
        *dest = fcitx_utils_escape_char(*pos);
        dest++;
        src = pos + 1;
    }
    if (len)
        memcpy(dest, src, len);
    dest += len;
    *dest = '\0';
    res = realloc(res, dest - res + 1);
    return res;
}

#ifdef __FCITX_ATOMIC_USE_SYNC_FETCH
/**
 * Also define lib function when there is builtin function for
 * atomic operation in case the function address is needed or the builtin
 * is not available when compiling other modules.
 **/
#define FCITX_UTIL_DEFINE_ATOMIC(name, op, type)                        \
    FCITX_EXPORT_API type                                               \
    (fcitx_utils_atomic_##name)(volatile type *atomic, type val)        \
    {                                                                   \
        return __sync_fetch_and_##name(atomic, val);                    \
    }
#else
static pthread_mutex_t __fcitx_utils_atomic_lock = PTHREAD_MUTEX_INITIALIZER;
#define FCITX_UTIL_DEFINE_ATOMIC(name, op, type)                        \
    FCITX_EXPORT_API type                                               \
    (fcitx_utils_atomic_##name)(volatile type *atomic, type val)        \
    {                                                                   \
        type oldval;                                                    \
        pthread_mutex_lock(&__fcitx_utils_atomic_lock);                 \
        oldval = *atomic;                                               \
        *atomic = oldval op val;                                        \
        pthread_mutex_unlock(&__fcitx_utils_atomic_lock);               \
        return oldval;                                                  \
    }
#endif

FCITX_UTIL_DEFINE_ATOMIC(add, +, int32_t)
FCITX_UTIL_DEFINE_ATOMIC(and, &, uint32_t)
FCITX_UTIL_DEFINE_ATOMIC(or, |, uint32_t)
FCITX_UTIL_DEFINE_ATOMIC(xor, ^, uint32_t)

#undef FCITX_UTIL_DEFINE_ATOMIC

// kate: indent-mode cstyle; space-indent on; indent-width 0;
