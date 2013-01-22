/***************************************************************************
 *   Copyright (C) 2012~2012 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
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
#ifndef _FCITX_DESKTOP_PARSE_H_
#define _FCITX_DESKTOP_PARSE_H_

#include <stdint.h>
#include <fcitx-utils/utils.h>

typedef struct _FcitxDesktopFile FcitxDesktopFile;
typedef struct _FcitxDesktopGroup FcitxDesktopGroup;
typedef struct _FcitxDesktopEntry FcitxDesktopEntry;

#define DESKTOP_PADDING_LEN 6

typedef struct {
    FcitxDesktopGroup *(*new_group)(void *owner);
    void (*free_group)(void *owner, FcitxDesktopGroup *group);
    FcitxDesktopEntry *(*new_entry)(void *owner);
    void (*free_entry)(void *owner, FcitxDesktopEntry *entry);
    void *padding[DESKTOP_PADDING_LEN];
} FcitxDesktopVTable;

struct _FcitxDesktopFile {
    FcitxDesktopGroup *first;
    FcitxDesktopGroup *last;
    UT_array comments; /* comment at the end of the file */

    /* private */
    const FcitxDesktopVTable *vtable;
    FcitxDesktopGroup *groups;
    void *owner;
    void *padding[3];
};

struct _FcitxDesktopGroup {
    FcitxDesktopEntry *first;
    FcitxDesktopEntry *last;
    FcitxDesktopGroup *prev;
    FcitxDesktopGroup *next;
    char *name; /* Read-only */
    UT_array comments; /* comment before the group name */

    /* private */
    const FcitxDesktopVTable *vtable;
    FcitxDesktopEntry *entries;
    UT_hash_handle hh;
    uint32_t flags;
    void *owner;
    int32_t ref_count;
    void *padding[3];
};

struct _FcitxDesktopEntry {
    FcitxDesktopEntry *prev;
    FcitxDesktopEntry *next;
    char *name; /* Read-only */
    UT_array comments; /* comment before the entry */
    char *value;

    /* private */
    const FcitxDesktopVTable *vtable;
    UT_hash_handle hh;
    uint32_t flags;
    void *owner;
    int32_t ref_count;
    void *padding[3];
};

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Memory management
     **/
    boolean fcitx_desktop_file_init(FcitxDesktopFile *file,
                                    const FcitxDesktopVTable *vtable,
                                    void *owner);
    void fcitx_desktop_file_done(FcitxDesktopFile *file);
    FcitxDesktopGroup *fcitx_desktop_group_ref(FcitxDesktopGroup *group);
    void fcitx_desktop_group_unref(FcitxDesktopGroup *group);
    FcitxDesktopEntry *fcitx_desktop_entry_ref(FcitxDesktopEntry *entry);
    void fcitx_desktop_entry_unref(FcitxDesktopEntry *entry);

    /**
     * Read and Write
     **/
    boolean fcitx_desktop_file_load_fp(FcitxDesktopFile *file, FILE *fp);
    boolean fcitx_desktop_file_load(FcitxDesktopFile *file, const char *name);
    boolean fcitx_desktop_file_write_fp(const FcitxDesktopFile *file,
                                        FILE *fp);
    boolean fcitx_desktop_file_write(const FcitxDesktopFile *file,
                                     const char *name);

    /**
     * Operations on FcitxDesktopGroups in a FcitxDesktopFile.
     **/
    FcitxDesktopGroup *fcitx_desktop_file_find_group_with_len(
        const FcitxDesktopFile *file, const char *name, size_t name_len);
    FcitxDesktopGroup *fcitx_desktop_file_add_group_after_with_len(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, size_t name_len, boolean move);
    FcitxDesktopGroup *fcitx_desktop_file_add_group_before_with_len(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, size_t name_len, boolean move);
    boolean fcitx_desktop_file_delete_group(FcitxDesktopFile *file,
                                            FcitxDesktopGroup *group);

    /**
     * Operations on FcitxDesktopEntry's in a FcitxDesktopGroup
     **/
    FcitxDesktopEntry *fcitx_desktop_group_find_entry_with_len(
        const FcitxDesktopGroup *group, const char *name, size_t name_len);
    FcitxDesktopEntry *fcitx_desktop_group_add_entry_after_with_len(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, size_t name_len, boolean move);
    FcitxDesktopEntry *fcitx_desktop_group_add_entry_before_with_len(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, size_t name_len, boolean move);
    boolean fcitx_desktop_group_delete_entry(FcitxDesktopGroup *group,
                                             FcitxDesktopEntry *entry);

    /**
     * Simplified inline functions
     **/
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_find_group(const FcitxDesktopFile *file,
                                  const char *name)
    {
        return fcitx_desktop_file_find_group_with_len(file, name,
                                                      strlen(name));
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_find_entry(const FcitxDesktopGroup *group,
                                   const char *name)
    {
        return fcitx_desktop_group_find_entry_with_len(group, name,
                                                       strlen(name));
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_ensure_group_with_len(
        FcitxDesktopFile *file, const char *name, size_t name_len)
    {
        return fcitx_desktop_file_add_group_after_with_len(
            file, NULL, name, name_len, false);
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_ensure_group(FcitxDesktopFile *file, const char *name)
    {
        return fcitx_desktop_file_ensure_group_with_len(
            file, name, strlen(name));
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_move_group_after_with_len(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, size_t name_len)
    {
        return fcitx_desktop_file_add_group_after_with_len(
            file, group, name, name_len, true);
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_move_group_after(
        FcitxDesktopFile *file, FcitxDesktopGroup *group, const char *name)
    {
        return fcitx_desktop_file_move_group_after_with_len(
            file, group, name, strlen(name));
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_add_group_after(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, boolean move)
    {
        return fcitx_desktop_file_add_group_after_with_len(
            file, group, name, strlen(name), move);
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_move_group_before_with_len(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, size_t name_len)
    {
        return fcitx_desktop_file_add_group_before_with_len(
            file, group, name, name_len, true);
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_move_group_before(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name)
    {
        return fcitx_desktop_file_move_group_before_with_len(
            file, group, name, strlen(name));
    }
    static inline FcitxDesktopGroup*
    fcitx_desktop_file_add_group_before(
        FcitxDesktopFile *file, FcitxDesktopGroup *group,
        const char *name, boolean move)
    {
        return fcitx_desktop_file_add_group_before_with_len(
            file, group, name, strlen(name), move);
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_ensure_entry_with_len(
        FcitxDesktopGroup *group, const char *name, size_t name_len)
    {
        return fcitx_desktop_group_add_entry_after_with_len(
            group, NULL, name, name_len, false);
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_ensure_entry(FcitxDesktopGroup *group, const char *name)
    {
        return fcitx_desktop_group_ensure_entry_with_len(
            group, name, strlen(name));
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_move_entry_after_with_len(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, size_t name_len)
    {
        return fcitx_desktop_group_add_entry_after_with_len(
            group, entry, name, name_len, true);
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_move_entry_after(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name)
    {
        return fcitx_desktop_group_move_entry_after_with_len(
            group, entry, name, strlen(name));
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_add_entry_after(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, boolean move)
    {
        return fcitx_desktop_group_add_entry_after_with_len(
            group, entry, name, strlen(name), move);
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_move_entry_before_with_len(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, size_t name_len)
    {
        return fcitx_desktop_group_add_entry_before_with_len(
            group, entry, name, name_len, true);
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_move_entry_before(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry, const char *name)
    {
        return fcitx_desktop_group_move_entry_before_with_len(
            group, entry, name, strlen(name));
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_group_add_entry_before(
        FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
        const char *name, boolean move)
    {
        return fcitx_desktop_group_add_entry_before_with_len(
            group, entry, name, strlen(name), move);
    }

    static inline FcitxDesktopEntry*
    fcitx_desktop_entry_set_value_with_len(FcitxDesktopEntry *entry,
                                           const char *value, size_t len)
    {
        fcitx_utils_string_swap_with_len(&entry->value, value, len);
        return entry;
    }
    static inline FcitxDesktopEntry*
    fcitx_desktop_entry_set_value(FcitxDesktopEntry *entry, const char *value)
    {
        fcitx_utils_string_swap(&entry->value, value);
        return entry;
    }

#ifdef __cplusplus
}
#endif

#endif
