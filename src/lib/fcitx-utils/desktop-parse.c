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

#include <libintl.h>
#include "config.h"
#include "fcitx/fcitx.h"
#include "utils.h"
#include "utf8.h"
#include "log.h"
#include "desktop-parse.h"

#define blank_chars " \t\b\n\f\v\r"

enum {
    FX_DESKTOP_GROUP_UPDATED = 1 << 0,
};

enum {
    FX_DESKTOP_ENTRY_UPDATED = 1 << 0,
};

static const void *empty_vtable_padding[DESKTOP_PADDING_LEN] = {};

/**
 * Check if vtable is valid. (Padding has to be zero).
 **/
static boolean
fcitx_desktop_parse_check_vtable(const FcitxDesktopVTable *vtable)
{
    if (!vtable)
        return true;
    if (memcmp(vtable->padding, empty_vtable_padding,
               sizeof(empty_vtable_padding))) {
        FcitxLog(ERROR, "Padding in vtable is not 0.");
        return false;
    }
    return true;
}

/**
 * Create a new group with vtable that doesn't belongs to any file.
 **/
static FcitxDesktopGroup*
fcitx_desktop_parse_new_group(const FcitxDesktopVTable *vtable, void *owner)
{
    FcitxDesktopGroup *group;
    if (vtable && vtable->new_group) {
        group = vtable->new_group(owner);
        memset(group, 0, sizeof(FcitxDesktopGroup));
    } else {
        group = fcitx_utils_new(FcitxDesktopGroup);
    }
    group->vtable = vtable;
    group->owner = owner;
    group->ref_count = 1;
    return group;
}

/**
 * Create a new entry with vtable that doesn't belongs to any group
 **/
static FcitxDesktopEntry*
fcitx_desktop_parse_new_entry(const FcitxDesktopVTable *vtable, void *owner)
{
    FcitxDesktopEntry *entry;
    if (vtable && vtable->new_entry) {
        entry = vtable->new_entry(owner);
        memset(entry, 0, sizeof(FcitxDesktopEntry));
    } else {
        entry = fcitx_utils_new(FcitxDesktopEntry);
    }
    entry->vtable = vtable;
    entry->owner = owner;
    entry->ref_count = 1;
    return entry;
}

/**
 * Init a file
 **/
FCITX_EXPORT_API boolean
fcitx_desktop_file_init(FcitxDesktopFile *file,
                        const FcitxDesktopVTable *vtable, void *owner)
{
    if (!fcitx_desktop_parse_check_vtable(vtable))
        return false;
    memset(file, 0, sizeof(FcitxDesktopFile));
    utarray_init(&file->comments, fcitx_str_icd);
    file->vtable = vtable;
    file->owner = owner;
    return true;
}

static char*
_find_none_blank(char *str)
{
    return str + strspn(str, blank_chars);
}

/**
 * Reset Functions:
 *     Clear comments, flags and link. Ready to be reused.
 **/
static void
fcitx_desktop_entry_reset(FcitxDesktopEntry *entry)
{
    utarray_clear(&entry->comments);
    entry->flags = 0;
    entry->prev = NULL;
    entry->next = NULL;
}

static void
fcitx_desktop_group_reset(FcitxDesktopGroup *group)
{
    FcitxDesktopEntry *entry;
    utarray_clear(&group->comments);
    for (entry = group->first;entry;entry = entry->next)
        fcitx_desktop_entry_reset(entry);
    group->flags = 0;
    group->prev = NULL;
    group->next = NULL;
    group->first = NULL;
    group->last = NULL;
}

static void
fcitx_desktop_file_reset(FcitxDesktopFile *file)
{
    FcitxDesktopGroup *group;
    for (group = file->first;group;group = group->next)
        fcitx_desktop_group_reset(group);
    utarray_clear(&file->comments);
    file->first = NULL;
    file->last = NULL;
}

static size_t
fcitx_desktop_group_name_len(const char *str)
{
    size_t res = strcspn(str, "[]");
    if (str[res] == ']')
        return res;
    return 0;
}

static size_t
fcitx_desktop_entry_key_len(const char *str)
{
    size_t res = strcspn(str, "=");
    if (str[res] != '=')
        return 0;
    for (;res > 0;res--) {
        if (strchr(blank_chars, str[res - 1]))
            continue;
        break;
    }
    return res;
}

/**
 * Free a entry that is already unlinked and removed from the hash table.
 **/
static void
fcitx_desktop_entry_free(FcitxDesktopEntry *entry)
{
    free(entry->name);
    fcitx_utils_free(entry->value);
    utarray_done(&entry->comments);
    if (entry->vtable && entry->vtable->free_entry) {
        entry->vtable->free_entry(entry->owner, entry);
    } else {
        free(entry);
    }
}

FCITX_EXPORT_API void
fcitx_desktop_entry_unref(FcitxDesktopEntry *entry)
{
    if (fcitx_utils_atomic_add(&entry->ref_count, -1) <= 1) {
        fcitx_desktop_entry_free(entry);
    }
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_entry_ref(FcitxDesktopEntry *entry)
{
    fcitx_utils_atomic_add(&entry->ref_count, 1);
    return entry;
}

/**
 * Remove a entry from a group and decrease the ref count by 1.
 **/
static void
fcitx_desktop_group_hash_remove_entry(FcitxDesktopGroup *group,
                                      FcitxDesktopEntry *entry)
{
    HASH_DEL(group->entries, entry);
    entry->prev = NULL;
    entry->next = NULL;
    entry->hh.tbl = NULL;
    fcitx_desktop_entry_unref(entry);
}

/**
 * Remove all its entries from group and free it. group should be already
 * unlinked and removed from the hash table.
 **/
static void
fcitx_desktop_group_free(FcitxDesktopGroup *group)
{
    FcitxDesktopEntry *entry;
    FcitxDesktopEntry *next;
    for (entry = group->entries;entry;entry = next) {
        next = entry->hh.next;
        fcitx_desktop_group_hash_remove_entry(group, entry);
    }
    free(group->name);
    utarray_done(&group->comments);
    if (group->vtable && group->vtable->free_group) {
        group->vtable->free_group(entry->owner, group);
    } else {
        free(group);
    }
}

FCITX_EXPORT_API void
fcitx_desktop_group_unref(FcitxDesktopGroup *group)
{
    if (fcitx_utils_atomic_add(&group->ref_count, -1) <= 1) {
        fcitx_desktop_group_free(group);
    }
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_group_ref(FcitxDesktopGroup *group)
{
    fcitx_utils_atomic_add(&group->ref_count, 1);
    return group;
}

/**
 * Remove a group from a file and decrease the ref count by 1.
 **/
static void
fcitx_desktop_file_hash_remove_group(FcitxDesktopFile *file,
                                     FcitxDesktopGroup *group)
{
    HASH_DEL(file->groups, group);
    group->prev = NULL;
    group->next = NULL;
    group->hh.tbl = NULL;
    fcitx_desktop_group_unref(group);
}

/**
 * Remove all groups from the file and free comments
 **/
FCITX_EXPORT_API void
fcitx_desktop_file_done(FcitxDesktopFile *file)
{
    FcitxDesktopGroup *group;
    FcitxDesktopGroup *next;
    for (group = file->groups;group;group = next) {
        next = group->hh.next;
        fcitx_desktop_file_hash_remove_group(file, group);
    }
    utarray_done(&file->comments);
}

/**
 * Check and clear UPDATED flags, remove unused entries.
 **/
static void
fcitx_desktop_group_clean(FcitxDesktopGroup *group)
{
    FcitxDesktopEntry *entry;
    FcitxDesktopEntry *next;
    for (entry = group->entries;entry;entry = next) {
        next = entry->hh.next;
        if (entry->flags & FX_DESKTOP_ENTRY_UPDATED) {
            entry->flags &= ~(FX_DESKTOP_ENTRY_UPDATED);
        } else {
            fcitx_desktop_group_hash_remove_entry(group, entry);
        }
    }
}

/**
 * Check and clear UPDATED flags, remove unused groups.
 **/
static void
fcitx_desktop_file_clean(FcitxDesktopFile *file)
{
    FcitxDesktopGroup *group;
    FcitxDesktopGroup *next;
    for (group = file->groups;group;group = next) {
        next = group->hh.next;
        if (group->flags & FX_DESKTOP_GROUP_UPDATED) {
            group->flags &= ~(FX_DESKTOP_GROUP_UPDATED);
            fcitx_desktop_group_clean(group);
        } else {
            fcitx_desktop_file_hash_remove_group(file, group);
        }
    }
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_file_find_group_with_len(const FcitxDesktopFile *file,
                                       const char *name, size_t name_len)
{
    FcitxDesktopGroup *group = NULL;
    HASH_FIND(hh, file->groups, name, name_len, group);
    return group;
}

/**
 * Create a new group and add it to the hash table of file.
 **/
static FcitxDesktopGroup*
fcitx_desktop_file_hash_new_group(FcitxDesktopFile *file,
                                  const char *name, size_t name_len)
{
    FcitxDesktopGroup *new_group;
    new_group = fcitx_desktop_parse_new_group(file->vtable, file->owner);
    new_group->name = malloc(name_len + 1);
    memcpy(new_group->name, name, name_len);
    new_group->name[name_len] = '\0';
    utarray_init(&new_group->comments, fcitx_str_icd);
    HASH_ADD_KEYPTR(hh, file->groups, new_group->name, name_len, new_group);
    return new_group;
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_group_find_entry_with_len(const FcitxDesktopGroup *group,
                                        const char *name, size_t name_len)
{
    FcitxDesktopEntry *entry = NULL;
    HASH_FIND(hh, group->entries, name, name_len, entry);
    return entry;
}

/**
 * Create a new entry and add it to the hash table of group.
 **/
static FcitxDesktopEntry*
fcitx_desktop_group_hash_new_entry(FcitxDesktopGroup *group,
                                   const char *name, size_t name_len)
{
    FcitxDesktopEntry *new_entry;
    new_entry = fcitx_desktop_parse_new_entry(group->vtable, group->owner);
    new_entry->name = malloc(name_len + 1);
    memcpy(new_entry->name, name, name_len);
    new_entry->name[name_len] = '\0';
    utarray_init(&new_entry->comments, fcitx_str_icd);
    HASH_ADD_KEYPTR(hh, group->entries, new_entry->name, name_len, new_entry);
    return new_entry;
}

static void
fcitx_desktop_group_set_link(FcitxDesktopGroup *new_group,
                             FcitxDesktopGroup **prev_p,
                             FcitxDesktopGroup **next_p)
{
    new_group->next = *prev_p;
    new_group->prev = *next_p;
    *prev_p = new_group;
    *next_p = new_group;
}

static void
fcitx_desktop_group_unlink(FcitxDesktopFile *file, FcitxDesktopGroup *group)
{
    if (group->prev) {
        group->prev->next = group->next;
    } else {
        file->first = group->next;
    }
    if (group->next) {
        group->next->prev = group->prev;
    } else {
        file->last = group->prev;
    }
}

static void
fcitx_desktop_file_link_group_after(FcitxDesktopFile *file,
                                    FcitxDesktopGroup *prev_group,
                                    FcitxDesktopGroup *new_group)
{
    FcitxDesktopGroup **prev_p;
    if (!prev_group) {
        prev_p = &file->first;
    } else {
        prev_p = &prev_group->next;
    }
    fcitx_desktop_group_set_link(new_group, prev_p, &file->last);
}

static void
fcitx_desktop_file_link_group_before(FcitxDesktopFile *file,
                                     FcitxDesktopGroup *next_group,
                                     FcitxDesktopGroup *new_group)
{
    FcitxDesktopGroup **next_p;
    if (!next_group) {
        next_p = &file->last;
    } else {
        next_p = &next_group->prev;
    }
    fcitx_desktop_group_set_link(new_group, &file->first, next_p);
}

static void
fcitx_desktop_entry_set_link(FcitxDesktopEntry *new_entry,
                             FcitxDesktopEntry **prev_p,
                             FcitxDesktopEntry **next_p)
{
    new_entry->next = *prev_p;
    new_entry->prev = *next_p;
    *prev_p = new_entry;
    *next_p = new_entry;
}

static void
fcitx_desktop_entry_unlink(FcitxDesktopGroup *group, FcitxDesktopEntry *entry)
{
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        group->first = entry->next;
    }
    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        group->last = entry->prev;
    }
}

static void
fcitx_desktop_group_link_entry_after(FcitxDesktopGroup *group,
                                     FcitxDesktopEntry *prev_entry,
                                     FcitxDesktopEntry *new_entry)
{
    FcitxDesktopEntry **prev_p;
    if (!prev_entry) {
        prev_p = &group->first;
    } else {
        prev_p = &prev_entry->next;
    }
    fcitx_desktop_entry_set_link(new_entry, prev_p, &group->last);
}

static void
fcitx_desktop_group_link_entry_before(FcitxDesktopGroup *group,
                                      FcitxDesktopEntry *next_entry,
                                      FcitxDesktopEntry *new_entry)
{
    FcitxDesktopEntry **next_p;
    if (!next_entry) {
        next_p = &group->last;
    } else {
        next_p = &next_entry->prev;
    }
    fcitx_desktop_entry_set_link(new_entry, &group->first, next_p);
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_load_fp(FcitxDesktopFile *file, FILE *fp)
{
    if (!(file && fp))
        return false;
    char *buff = NULL;
    size_t buff_len = 0;
    int lineno = 0;
    fcitx_desktop_file_reset(file);
    UT_array *comments = &file->comments;
    FcitxDesktopGroup *cur_group = NULL;

    while (getline(&buff, &buff_len, fp) != -1) {
        size_t line_len = strcspn(buff, "\n");
        buff[line_len] = '\0';
        lineno++;
        char *line = _find_none_blank(buff);
        switch (*line) {
        case '#':
            line++;
            utarray_push_back(comments, &line);
        case '\0':
            continue;
        case '[': {
            line++;
            size_t name_len = fcitx_desktop_group_name_len(line);
            if (!name_len) {
                FcitxLog(ERROR, _("Invalid group name line @ line %d %s"),
                         lineno, line - 1);
                continue;
            }
            FcitxDesktopGroup *new_group;
            new_group = fcitx_desktop_file_find_group_with_len(file, line,
                                                               name_len);
            if (!new_group) {
                new_group = fcitx_desktop_file_hash_new_group(file, line,
                                                              name_len);
            } else {
                if (new_group->flags & FX_DESKTOP_GROUP_UPDATED) {
                    FcitxLog(WARNING, _("Duplicate group %s in a desktop file,"
                                        "@ line %d, merge with previous one."),
                             new_group->name, lineno);
                    if (new_group == cur_group)
                        cur_group = new_group->prev;
                    fcitx_desktop_group_unlink(file, new_group);
                    utarray_clear(&new_group->comments);
                } else {
                    new_group->first = NULL;
                    new_group->last = NULL;
                }
            }
            new_group->flags |= FX_DESKTOP_GROUP_UPDATED;
            fcitx_desktop_file_link_group_after(file, cur_group, new_group);
            UT_array tmp_ary = *comments;
            *comments = new_group->comments;
            new_group->comments = tmp_ary;
            cur_group = new_group;
            continue;
        }
        default:
            break;
        }
        if (!cur_group) {
            FcitxLog(ERROR, _("Non-comment doesn't belongs to any group "
                              "@ %d %s"), lineno, line);
            continue;
        }
        size_t key_len = fcitx_desktop_entry_key_len(line);
        if (!key_len) {
            FcitxLog(ERROR, _("Invalid entry line @ line %d %s"), lineno, line);
            continue;
        }
        FcitxDesktopEntry *new_entry;
        new_entry = fcitx_desktop_group_find_entry_with_len(cur_group,
                                                            line, key_len);
        if (!new_entry) {
            new_entry = fcitx_desktop_group_hash_new_entry(cur_group,
                                                           line, key_len);
        } else if (new_entry->flags & FX_DESKTOP_ENTRY_UPDATED) {
            FcitxLog(ERROR, _("Duplicate entry %s in group %s,"
                              "@ line %d, override previous one."),
                     new_entry->name, cur_group->name, lineno);
            fcitx_desktop_entry_unlink(cur_group, new_entry);
            fcitx_desktop_entry_reset(new_entry);
        }
        new_entry->flags |= FX_DESKTOP_ENTRY_UPDATED;
        fcitx_desktop_group_link_entry_after(cur_group, cur_group->last,
                                             new_entry);
        UT_array tmp_ary = *comments;
        *comments = new_entry->comments;
        new_entry->comments = tmp_ary;
        char *value = line + key_len + 1;
        int value_len = buff + line_len - value;
        new_entry->value = fcitx_utils_set_str_with_len(new_entry->value,
                                                        value, value_len);
    }
    fcitx_desktop_file_clean(file);
    fcitx_utils_free(buff);
    return true;
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_load(FcitxDesktopFile *file, const char *name)
{
    boolean res;
    FILE *fp = fopen(name, "r");
    res = fcitx_desktop_file_load_fp(file, fp);
    if (fp)
        fclose(fp);
    return res;
}

static inline size_t
_write_len(FILE *fp, char *str, size_t len)
{
    if (!(str && len))
        return 0;
    return fwrite(str, len, 1, fp);
}

static inline size_t
_write_str(FILE *fp, char *str)
{
    return _write_len(fp, str, strlen(str));
}

static size_t
_check_single_line(char *str)
{
    if (!str)
        return 0;
    size_t len = strcspn(str, "\n");
    if (str[len])
        FcitxLog(ERROR, "Not a single line, ignore.");
    return len;
}

static void
fcitx_desktop_write_comments(FILE *fp, const UT_array *comments)
{
    utarray_foreach(comment, comments, char*) {
        size_t len = _check_single_line(*comment);
        if (!len)
            continue;
        _write_str(fp, "#");
        _write_len(fp, *comment, len);
        _write_str(fp, "\n");
    }
}

static size_t
_check_entry_key(char *str)
{
    if (!str)
        return 0;
    size_t len = strcspn(str, "=\n");
    if (str[len]) {
        FcitxLog(ERROR, "Not a valid key, skip.");
        return 0;
    }
    switch (str[len - 1]) {
    case ' ':
    case '\r':
    case '\t':
    case '\f':
    case '\v':
        FcitxLog(ERROR, "Not a valid key, skip.");
        return 0;
    }
    return len;
}

static void
fcitx_desktop_entry_write_fp(const FcitxDesktopEntry *entry, FILE *fp)
{
    if (!entry->value)
        return;
    size_t key_len = _check_entry_key(entry->name);
    if (!key_len)
        return;
    size_t value_len = _check_single_line(entry->value);
    fcitx_desktop_write_comments(fp, &entry->comments);
    _write_len(fp, entry->name, key_len);
    _write_str(fp, "=");
    _write_len(fp, entry->value, value_len);
    _write_str(fp, "\n");
}

static size_t
_check_group_name(char *str)
{
    if (!str)
        return 0;
    size_t len = strcspn(str, "[]\n");
    if (str[len]) {
        FcitxLog(ERROR, "Not a valid group name, skip.");
        return 0;
    }
    return len;
}

static void
fcitx_desktop_group_write_fp(const FcitxDesktopGroup *group, FILE *fp)
{
    FcitxDesktopEntry *entry;
    size_t name_len = _check_group_name(group->name);
    if (!name_len)
        return;
    fcitx_desktop_write_comments(fp, &group->comments);
    _write_str(fp, "[");
    _write_len(fp, group->name, name_len);
    _write_str(fp, "]\n");
    for (entry = group->first;entry;entry = entry->next) {
        fcitx_desktop_entry_write_fp(entry, fp);
    }
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_write_fp(const FcitxDesktopFile *file, FILE *fp)
{
    if (!(file && fp))
        return false;
    FcitxDesktopGroup *group;
    for (group = file->first;group;group = group->next) {
        fcitx_desktop_group_write_fp(group, fp);
    }
    fcitx_desktop_write_comments(fp, &file->comments);
    return true;
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_write(const FcitxDesktopFile *file, const char *name)
{
    boolean res;
    FILE *fp = fopen(name, "w");
    res = fcitx_desktop_file_write_fp(file, fp);
    if (fp)
        fclose(fp);
    return res;
}

static inline boolean
_fcitx_desktop_file_has_group(FcitxDesktopFile *file, FcitxDesktopGroup *group)
{
    if (fcitx_unlikely(!file->groups || !group))
        return false;
    return file->groups->hh.tbl == group->hh.tbl;
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_delete_group(FcitxDesktopFile *file,
                                FcitxDesktopGroup *group)
{
    if (!_fcitx_desktop_file_has_group(file, group))
        return false;
    fcitx_desktop_group_unlink(file, group);
    fcitx_desktop_file_hash_remove_group(file, group);
    return true;
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_file_add_group_after_with_len(
    FcitxDesktopFile *file, FcitxDesktopGroup *group,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopGroup *new_group;
    if (group) {
        if (!_fcitx_desktop_file_has_group(file, group)) {
            FcitxLog(ERROR,
                     "The given group doesn't belong to the given file.");
            return NULL;
        }
    } else {
        group = file->last;
    }
    new_group =  fcitx_desktop_file_find_group_with_len(file, name, name_len);
    if (!new_group) {
        new_group = fcitx_desktop_file_hash_new_group(file, name, name_len);
        fcitx_desktop_file_link_group_after(file, group, new_group);
    } else if (move && !(new_group == group)) {
        fcitx_desktop_group_unlink(file, new_group);
        fcitx_desktop_file_link_group_after(file, group, new_group);
    }
    return new_group;
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_file_add_group_before_with_len(
    FcitxDesktopFile *file, FcitxDesktopGroup *group,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopGroup *new_group;
    if (group) {
        if (!_fcitx_desktop_file_has_group(file, group)) {
            FcitxLog(ERROR,
                     "The given group doesn't belong to the given file.");
            return NULL;
        }
    } else {
        group = file->first;
    }
    new_group =  fcitx_desktop_file_find_group_with_len(file, name, name_len);
    if (!new_group) {
        new_group = fcitx_desktop_file_hash_new_group(file, name, name_len);
        fcitx_desktop_file_link_group_before(file, group, new_group);
    } else if (move && !(new_group == group)) {
        fcitx_desktop_group_unlink(file, new_group);
        fcitx_desktop_file_link_group_before(file, group, new_group);
    }
    return new_group;
}

static inline boolean
_fcitx_desktop_group_has_entry(FcitxDesktopGroup *group,
                               FcitxDesktopEntry *entry)
{
    if (fcitx_unlikely(!group->entries || !entry))
        return false;
    return group->entries->hh.tbl == entry->hh.tbl;
}

FCITX_EXPORT_API boolean
fcitx_desktop_group_delete_entry(FcitxDesktopGroup *group,
                                 FcitxDesktopEntry *entry)
{
    if (!_fcitx_desktop_group_has_entry(group, entry))
        return false;
    fcitx_desktop_entry_unlink(group, entry);
    fcitx_desktop_group_hash_remove_entry(group, entry);
    return true;
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_group_add_entry_after_with_len(
    FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopEntry *new_entry;
    if (entry) {
        if (!_fcitx_desktop_group_has_entry(group, entry)) {
            FcitxLog(ERROR,
                     "The given entry doesn't belong to the given group.");
            return NULL;
        }
    } else {
        entry = group->last;
    }
    new_entry =  fcitx_desktop_group_find_entry_with_len(group, name, name_len);
    if (!new_entry) {
        new_entry = fcitx_desktop_group_hash_new_entry(group, name, name_len);
        fcitx_desktop_group_link_entry_after(group, entry, new_entry);
    } else if (move && !(new_entry == entry)) {
        fcitx_desktop_entry_unlink(group, new_entry);
        fcitx_desktop_group_link_entry_after(group, entry, new_entry);
    }
    return new_entry;
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_group_add_entry_before_with_len(
    FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopEntry *new_entry;
    if (entry) {
        if (!_fcitx_desktop_group_has_entry(group, entry)) {
            FcitxLog(ERROR,
                     "The given entry doesn't belong to the given group.");
            return NULL;
        }
    } else {
        entry = group->first;
    }
    new_entry =  fcitx_desktop_group_find_entry_with_len(group, name, name_len);
    if (!new_entry) {
        new_entry = fcitx_desktop_group_hash_new_entry(group, name, name_len);
        fcitx_desktop_group_link_entry_before(group, entry, new_entry);
    } else if (move && !(new_entry == entry)) {
        fcitx_desktop_entry_unlink(group, new_entry);
        fcitx_desktop_group_link_entry_before(group, entry, new_entry);
    }
    return new_entry;
}
