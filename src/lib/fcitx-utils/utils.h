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
 * @defgroup FcitxUtils FcitxUtils
 *
 * Fcitx Utils contains a bunch of functions, with extend common c library.
 * It contains uthash/utarray as macro based hash table and dynamic array.
 *
 * Some Fcitx path related function, includes fcitx_utils_get_fcitx_path()
 * and fcitx_utils_get_fcitx_path_with_filename()
 *
 * Some string related function, providing split, join string list.
 *
 * A simple memory pool, if you need to allocate many small memory block, and
 * only need to free them at once.
 *
 * Some log function, which prints the current file number, with printf format.
 *
 * Why Fcitx does not use GLib?
 *
 * The answer is glib contains a lot of function that fcitx doesn't need, and
 * in order to keep fcitx-core only depends on libc,
 * we provide some functions that are equivalent to glib ones in fcitx-utils.
 */

/**
 * @addtogroup FcitxUtils
 * @{
 */

/**
 * @file   utils.h
 * @author CS Slayer wengxt@gmail.com
 *
 *  Misc function for Fcitx.
 *
 */

#ifndef _FCITX_UTILS_H_
#define _FCITX_UTILS_H_

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-utils/uthash.h>
#include <sys/stat.h>

/**
 * fcitx boolean
 **/
typedef int32_t boolean;
/**
 * fcitx true
 */
#define true (1)
/**
 * fcitx false
 */
#define false (0)

#define FCITX_INT_LEN ((int)(sizeof(int) * 2.5) + 2)
#define FCITX_LONG_LEN ((int)(sizeof(long) * 2.5) + 2)
#define FCITX_INT32_LEN (22)
#define FCITX_INT64_LEN (42)

#define fcitx_container_of(ptr, type, member)           \
    ((type*)(((void*)(ptr)) - offsetof(type, member)))

#ifdef __cplusplus
extern "C" {
#endif

    extern const UT_icd *const fcitx_str_icd;
    extern const UT_icd *const fcitx_int_icd;

    /**
     * Function used to free the pointer
     **/
    typedef void (*FcitxDestroyNotify)(void*);
    /**
     * Function used to free the content of a structure,
     * DO NOT free the pointer itself
     **/
    typedef void (*FcitxFreeContentFunc)(void*);
    typedef void (*FcitxCallBack)();
    /**
     * A hash set for string
     **/
    typedef struct _FcitxStringHashSet {
        /**
         * String in Hash Set
         **/
        char *name;
        /**
         * UT Hash handle
         **/
        UT_hash_handle hh;
    } FcitxStringHashSet;


    /**
     * Custom bsearch, it can search the most near value.
     *
     * @param key
     * @param base
     * @param nmemb
     * @param size
     * @param accurate
     * @param compar
     *
     * @return
     */
    void *fcitx_utils_custom_bsearch(const void *key, const void *base,
                                     size_t nmemb, size_t size, int accurate,
                                     int (*compar)(const void *, const void *));

    /**
     * Fork twice to run as daemon
     *
     * @return void
     **/
    void fcitx_utils_init_as_daemon(void);

    /**
     * Count the file line count
     *
     * @param fpDict file pointer
     * @return int line count
     **/
    int fcitx_utils_calculate_record_number(FILE* fpDict);


    /**
     * create empty string list
     *
     * @return UT_array*
     **/
    UT_array* fcitx_utils_new_string_list(void);

    /**
     * Split a string by delm
     *
     * @param str input string
     * @param delm character as delimiter
     * @return UT_array* a new utarray for store the split string
     **/
    UT_array* fcitx_utils_split_string(const char *str, char delm);

    /**
     * append a string with printf format
     *
     * @param list string list
     * @param fmt printf fmt
     * @return void
     **/
    void fcitx_utils_string_list_printf_append(UT_array* list, const char* fmt,...);

    /**
     * Join string list with delm
     *
     * @param list string list
     * @param delm delm
     * @return char* return string, need to be free'd
     **/
    char* fcitx_utils_join_string_list(UT_array* list, char delm);

    /**
     * check if a string list contains a specific string
     *
     * @param list string list
     * @param scmp string to compare
     *
     * @return 1 for found, 0 for not found.
     *
     * @since 4.2.5
     */
    int fcitx_utils_string_list_contains(UT_array* list, const char* scmp);

    /**
     * Helper function for free the SplitString Output
     *
     * @param list the SplitString Output
     * @return void
     * @see fcitx_utils_split_string
     **/
    void fcitx_utils_free_string_list(UT_array *list);

    /**
     * Free String Hash Set
     *
     * @param sset String Hash Set
     * @return void
     *
     * @since 4.2.0
     **/
    void fcitx_utils_free_string_hash_set(FcitxStringHashSet* sset);

    /**
     * Trim the input string's white space
     *
     * @param s input string
     * @return char* new malloced string, need to free'd by caller
     **/
    char* fcitx_utils_trim(const char *s);

    /**
     * Malloc and memset all memory to zero
     *
     * @param bytes malloc size
     * @return void* malloced pointer
     **/
    void* fcitx_utils_malloc0(size_t bytes);

#define fcitx_utils_new(TYPE) ((TYPE*) fcitx_utils_malloc0(sizeof(TYPE)))

    /**
     * Get Display number, Fcitx DBus and Socket are identified by display number.
     *
     * @return int
     **/
    int fcitx_utils_get_display_number(void);

    /**
     * Get current language code, result need to be free'd
     * It will check LC_CTYPE, LC_ALL, LANG, for current language code.
     *
     * @return char*
     **/
    char* fcitx_utils_get_current_langcode(void);

    /**
     * check the current locale is utf8 or not
     *
     * @return int
     **/
    int fcitx_utils_current_locale_is_utf8(void);

    /**
     * Get Current Process Name, implementation depends on OS,
     * Always return a string need to be free'd, if cannot get
     * current process name, it will return "".
     *
     * @return char*
     **/
    char* fcitx_utils_get_process_name(void);


    /**
     * @brief check a process is running or not
     *
     * @param pid pid
     * @return 1 for exists or error, 0 for non exists
     **/
    int fcitx_utils_pid_exists(pid_t pid);

    /**
     * Get Fcitx install path, need be free'd
     * All possible type includes:
     * datadir
     * pkgdatadir
     * bindir
     * libdir
     * localedir
     *
     * It's determined at compile time, and can be changed via environment variable: FCITXDIR
     *
     * It will only return NULL while the type is invalid.
     *
     * @param type path type
     *
     * @return char*
     *
     * @since 4.2.1
     */
    char* fcitx_utils_get_fcitx_path(const char* type);

    /**
     * Get fcitx install path with file name, need to be free'd
     *
     * It's just simply return the path/filename string.
     *
     * It will only return NULL while the type is invalid.
     *
     * @param type path type
     * @param filename filename
     *
     * @return char*
     *
     * @see fcitx_utils_get_fcitx_path
     *
     * @since 4.2.1
     */
    char* fcitx_utils_get_fcitx_path_with_filename(const char* type, const char* filename);

    /**
     * lanunch fcitx's tool
     *
     * @param name tool's name
     * @param arg single arg
     *
     * @return void
     *
     * @since 4.2.6
     */
    void fcitx_utils_launch_tool(const char* name, const char* arg);

    /**
     * launch fcitx-configtool
     *
     * @return void
     **/
    void fcitx_utils_launch_configure_tool(void);

    /**
     * launch fcitx-configtool for an addon
     *
     * @return void
     **/
    void fcitx_utils_launch_configure_tool_for_addon(const char* addon);

    /**
     * helper function to execute fcitx -r
     *
     * @return void
     **/
    void fcitx_utils_launch_restart(void);

    /**
     * @brief launch a process
     *
     * @param args argument and command
     * @return void
     *
     * @since 4.2.5
     **/
    void fcitx_utils_start_process(char** args);

    /**
     * output backtrace to stderr, need to enable backtrace, this function
     * will be signal safe since 4.2.7, if you want to use it in debug,
     * you'd better call fflush(stderr) before call to it.
     *
     * @return void
     *
     * @since 4.2.5
     **/
    void fcitx_utils_backtrace();

    /**
     * @brief get bool environment var for convinience.
     *
     * @param name var name
     * @param defval default value
     *
     * @return value of var
     *
     * @since 4.2.6
     */
    int fcitx_utils_get_boolean_env(const char *name, int defval);

    /**
     * if obj is null, free it, after that, if str is NULL set it with NULL,
     * if str is not NULL, set it with strdup(str)
     *
     * @param obj object string
     * @param str source string
     * @return void
     **/
    void fcitx_utils_string_swap(char** obj, const char* str);
    void fcitx_utils_string_swap_with_len(char** obj,
                                          const char* str, size_t len);

    /**
     * similar with strcmp, but can handle the case that a or b is null.
     * NULL < not NULL and NULL == NULL
     *
     * @param a string a
     * @param b string b
     * @return same as rule of strcmp
     */
    int fcitx_utils_strcmp0(const char* a, const char* b);

    /**
     * similiar with fcitx_utils_strcmp0, but empty string will be considered
     * equals to NULL in this case.
     * NULL == empty, and empty < not empty
     *
     * @param a string a
     * @param b string b
     * @return same as rule of strcmp
     */
    int fcitx_utils_strcmp_empty(const char* a, const char* b);

    /** free a pointer if it's not NULL */
    static inline void
    fcitx_utils_free(void *ptr)
    {
        if (ptr)
            free(ptr);
    }

#define fcitx_utils_read(fp, p, type)           \
    fcitx_utils_read_ ## type(fp, p)

#define fcitx_utils_write(fp, p, type)          \
    fcitx_utils_write_ ## type(fp, p)

    /**
     * read a little endian 32bit unsigned int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_read_uint32(FILE *fp, uint32_t *p);

    /**
     * read a little endian 32bit int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_read_int32(FILE *fp, int32_t *p)
    {
        return fcitx_utils_read_uint32(fp, (uint32_t*)p);
    }

    /**
     * write a little endian 32bit int to a file
     *
     * @param fp FILE* to write to
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_write_uint32(FILE *fp, uint32_t i);

    /**
     * write a little endian 32bit unsigned int to a file
     *
     * @param fp FILE* to write to
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_write_int32(FILE *fp, int32_t i)
    {
        return fcitx_utils_write_uint32(fp, (uint32_t)i);
    }


    /**
     * read a little endian 64bit unsigned int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_read_uint64(FILE *fp, uint64_t *p);

    /**
     * read a little endian 64bit int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_read_int64(FILE *fp, int64_t *p)
    {
        return fcitx_utils_read_uint64(fp, (uint64_t*)p);
    }

    /**
     * write a little endian 64bit int to a file
     *
     * @param fp FILE* to write
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_write_uint64(FILE *fp, uint64_t i);

    /**
     * write a little endian 64bit unsigned int to a file
     *
     * @param fp FILE* to write to
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_write_int64(FILE *fp, int64_t i)
    {
        return fcitx_utils_write_uint64(fp, (uint64_t)i);
    }


    /**
     * read a little endian 16bit unsigned int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_read_uint16(FILE *fp, uint16_t *p);

    /**
     * read a little endian 16bit int from a file
     *
     * @param fp FILE* to read from
     * @param p return the integer read
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_read_int16(FILE *fp, int16_t *p)
    {
        return fcitx_utils_read_uint16(fp, (uint16_t*)p);
    }

    /**
     * write a little endian 16bit int to a file
     *
     * @param fp FILE* to write to
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    size_t fcitx_utils_write_uint16(FILE *fp, uint16_t i);

    /**
     * write a little endian 16bit unsigned int to a file
     *
     * @param fp FILE* to write to
     * @param i int to write in host endian
     * @return 1 on success, 0 on error
     * @since 4.2.6
     **/
    static inline size_t
    fcitx_utils_write_int16(FILE *fp, int16_t i)
    {
        return fcitx_utils_write_uint16(fp, (uint16_t)i);
    }

    size_t fcitx_utils_str_lens(size_t n, const char **str_list,
                                size_t *size_list);
    void fcitx_utils_cat_str(char *out, size_t n, const char **str_list,
                             const size_t *size_list);
    void fcitx_utils_cat_str_with_len(char *out, size_t len, size_t n,
                                      const char **str_list,
                                      const size_t *size_list);
#define fcitx_utils_cat_str_simple(out, n, str_list) do {       \
        size_t __tmp_size_list[n];                              \
        fcitx_utils_str_lens(n, str_list, __tmp_size_list);     \
        fcitx_utils_cat_str(out, n, str_list, __tmp_size_list); \
    } while (0)

#define fcitx_utils_cat_str_simple_with_len(out, len, n, str_list) do { \
        size_t __tmp_size_list[n];                                      \
        fcitx_utils_str_lens(n, str_list, __tmp_size_list);             \
        fcitx_utils_cat_str_with_len(out, len, n, str_list, __tmp_size_list); \
    } while (0)

#define fcitx_utils_local_cat_str(dest, len, strs...)                   \
    const char *__str_list_##dest[] = {strs};                           \
    size_t __size_list_##dest[sizeof((const char*[]){strs}) / sizeof(char*)]; \
    fcitx_utils_str_lens(sizeof((const char*[]){strs}) / sizeof(char*), \
                         __str_list_##dest, __size_list_##dest);        \
    char dest[len];                                                     \
    fcitx_utils_cat_str_with_len(dest, len,                             \
                                 sizeof((const char*[]){strs}) / sizeof(char*), \
                                 __str_list_##dest, __size_list_##dest)

#define fcitx_utils_alloc_cat_str(dest, strs...) do {                   \
        const char *__str_list[] = {strs};                              \
        size_t __cat_str_n = sizeof(__str_list) / sizeof(char*);        \
        size_t __size_list[sizeof(__str_list) / sizeof(char*)];         \
        size_t __total_size = fcitx_utils_str_lens(__cat_str_n,         \
                                                   __str_list, __size_list); \
        dest = malloc(__total_size);                                    \
        fcitx_utils_cat_str(dest, __cat_str_n,                          \
                            __str_list, __size_list);                   \
    } while (0)

#define fcitx_utils_set_cat_str(dest, strs...) do {                     \
        const char *__str_list[] = {strs};                              \
        size_t __cat_str_n = sizeof(__str_list) / sizeof(char*);        \
        size_t __size_list[sizeof(__str_list) / sizeof(char*)];         \
        size_t __total_size = fcitx_utils_str_lens(__cat_str_n,         \
                                                   __str_list, __size_list); \
        dest = realloc(dest, __total_size);                             \
        fcitx_utils_cat_str(dest, __cat_str_n,                          \
                            __str_list, __size_list);                   \
    } while (0)

    static inline int fcitx_utils_isdir(const char *path)
    {
        struct stat stats;
        return (stat(path, &stats) == 0 && S_ISDIR(stats.st_mode) &&
                access(path, R_OK | X_OK) == 0);
    }

    static inline int fcitx_utils_isreg(const char *path)
    {
        struct stat stats;
        return (stat(path, &stats) == 0 && S_ISREG(stats.st_mode) &&
                access(path, R_OK) == 0);
    }

    static inline int fcitx_utils_islnk(const char *path)
    {
        struct stat stats;
        return stat(path, &stats) == 0 && S_ISLNK(stats.st_mode);
    }
    char *fcitx_utils_set_str_with_len(char *res, const char *str, size_t len);
    static inline char*
    fcitx_utils_set_str(char *res, const char *str)
    {
        return fcitx_utils_set_str_with_len(res, str, strlen(str));
    }
    char fcitx_utils_unescape_char(char c);
    char *fcitx_utils_unescape_str_inplace(char *str);
    char *fcitx_utils_set_unescape_str(char *res, const char *str);
#define FCITX_CHAR_NEED_ESCAPE "\a\b\f\n\r\t\e\v\'\"\\"
    char fcitx_utils_escape_char(char c);
    char *fcitx_utils_set_escape_str_with_set(char *res, const char *str,
                                              const char *set);
    static inline char*
    fcitx_utils_set_escape_str(char *res, const char *str)
    {
        return fcitx_utils_set_escape_str_with_set(res, str, NULL);
    }
    UT_array *fcitx_utils_append_split_string(UT_array *list, const char* str,
                                              const char *delm);
    static inline UT_array*
    fcitx_utils_append_lines(UT_array *list, const char* str)
    {
        return fcitx_utils_append_split_string(list, str, "\n");
    }
    UT_array *fcitx_utils_string_list_append_no_copy(UT_array *list, char *str);
    UT_array *fcitx_utils_string_list_append_len(UT_array *list,
                                                 const char *str, size_t len);
#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */

// kate: indent-mode cstyle; space-indent on; indent-width 0;
