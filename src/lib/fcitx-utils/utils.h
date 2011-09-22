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
    typedef struct _StringHashSet {
        /**
         * @brief String in Hash Set
         **/
        char *name;
        /**
         * @brief UT Hash handle
         **/
        UT_hash_handle hh;
    } StringHashSet;


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
    void *custom_bsearch(const void *key, const void *base,
                         size_t nmemb, size_t size, int accurate,
                         int (*compar)(const void *, const void *));

    /**
     * @brief Fork twice to run as daemon
     *
     * @return void
     **/
    void InitAsDaemon();

    /**
     * @brief Count the file line number
     *
     * @param fpDict file pointer
     * @return int line number
     **/
    int CalculateRecordNumber(FILE* fpDict);

    /**
     * @brief Split a string by delm
     *
     * @param str input string
     * @param delm character as delimiter
     * @return UT_array* a new utarray for store the split string
     **/
    UT_array* SplitString(const char *str, char delm);

    /**
     * @brief Helper function for free the SplitString Output
     *
     * @param list the SplitString Output
     * @return void
     * @see SplitString
     **/
    void FreeStringList(UT_array *list);

    /**
     * @brief Trim the input string's white space
     *
     * @param s input string
     * @return char* new malloced string, need to free'd by caller
     **/
    char* fcitx_trim(char *s);

    /**
     * @brief Malloc and memset all memory to zero
     *
     * @param bytes malloc size
     * @return void* malloced pointer
     **/
    void* fcitx_malloc0(size_t bytes);

    int FcitxGetDisplayNumber();

    char* fcitx_get_process_name();

#ifdef __cplusplus
}
#endif

#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
