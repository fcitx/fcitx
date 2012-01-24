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
 * @file   utils.h
 * @author CS Slayer wengxt@gmail.com
 *
 * @brief  Misc function for Fcitx
 *
 */

#ifndef _FCITX_UTILS_H_
#define _FCITX_UTILS_H_

#include <stdio.h>
#include <fcitx-utils/utarray.h>
#include <fcitx-utils/uthash.h>

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief A hash set for string
     **/
    typedef struct _FcitxStringHashSet {
        /**
         * @brief String in Hash Set
         **/
        char *name;
        /**
         * @brief UT Hash handle
         **/
        UT_hash_handle hh;
    } FcitxStringHashSet;


    /**
     * @brief Custom bsearch, it can search the most near value.
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
     * @brief Fork twice to run as daemon
     *
     * @return void
     **/
    void fcitx_utils_init_as_daemon();

    /**
     * @brief Count the file line number
     *
     * @param fpDict file pointer
     * @return int line number
     **/
    int fcitx_utils_calculate_record_number(FILE* fpDict);
    
    
    /**
     * @brief new empty string list
     *
     * @return UT_array*
     **/
    UT_array* fcitx_utils_new_string_list();

    /**
     * @brief Split a string by delm
     *
     * @param str input string
     * @param delm character as delimiter
     * @return UT_array* a new utarray for store the split string
     **/
    UT_array* fcitx_utils_split_string(const char *str, char delm);
    
    /**
     * @brief append a string with printf format
     *
     * @param list string list
     * @param fmt printf fmt
     * @return void
     **/
    void fcitx_utils_string_list_printf_append(UT_array* list, const char* fmt,...);
    
    /**
     * @brief Join string list with delm
     *
     * @param list string list
     * @param delm delm
     * @return char* return string, need to be free'd
     **/
    char* fcitx_utils_join_string_list(UT_array* list, char delm);

    /**
     * @brief Helper function for free the SplitString Output
     *
     * @param list the SplitString Output
     * @return void
     * @see SplitString
     **/
    void fcitx_utils_free_string_list(UT_array *list);

    /**
     * @brief Free String Hash Set
     *
     * @param sset String Hash Set
     * @return void
     *
     * @since 4.1.3
     **/
    void fcitx_utils_free_string_hash_set(FcitxStringHashSet* sset);

    /**
     * @brief Trim the input string's white space
     *
     * @param s input string
     * @return char* new malloced string, need to free'd by caller
     **/
    char* fcitx_utils_trim(const char *s);

    /**
     * @brief Malloc and memset all memory to zero
     *
     * @param bytes malloc size
     * @return void* malloced pointer
     **/
    void* fcitx_utils_malloc0(size_t bytes);

    /**
     * @brief Get Display number
     *
     * @return int
     **/
    int fcitx_utils_get_display_number();
    
    /**
     * @brief Get current language code, result need to be free'd
     *
     * @return char*
     **/
    char* fcitx_utils_get_current_langcode();

    /**
     * @brief Get Current Process Name
     *
     * @return char*
     **/
    char* fcitx_utils_get_process_name();

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
