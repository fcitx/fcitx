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
#include <unistd.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-utils/uthash.h>

#ifdef __cplusplus
extern "C" {
#endif

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
     * Helper function for free the SplitString Output
     *
     * @param list the SplitString Output
     * @return void
     * @see SplitString
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
     * if obj is null, free it, after that, if str is NULL set it with NULL,
     * if str is not NULL, set it with strdup(str)
     *
     * @param obj object string
     * @param str source string
     * @return void
     **/
    void fcitx_utils_string_swap(char** obj, const char* str);

    /** free a pointer if it's not NULL */
#define fcitx_utils_free(ptr) do { \
        if (ptr) \
            free(ptr); \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif

/**
 * @}
 */

// kate: indent-mode cstyle; space-indent on; indent-width 0;
