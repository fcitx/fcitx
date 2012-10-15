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

static boolean
fcitx_desktop_parse_check_vtable(FcitxDesktopVTable *vtable)
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

static FcitxDesktopGroup*
fcitx_desktop_parse_new_group(FcitxDesktopVTable *vtable, void *owner)
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
    return group;
}

static FcitxDesktopEntry*
fcitx_desktop_parse_new_entry(FcitxDesktopVTable *vtable, void *owner)
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
    return entry;
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_init(FcitxDesktopFile *file, FcitxDesktopVTable *vtable,
                        void *owner)
{
    if (!fcitx_desktop_parse_check_vtable(vtable))
        return false;
    memset(file, 0, sizeof(FcitxDesktopFile));
    file->vtable = vtable;
    file->owner = owner;
    utarray_init(&file->comments, fcitx_str_icd);
    return true;
}

static char*
_find_none_blank(char *str)
{
    return str + strspn(str, blank_chars);
}

static void
fcitx_desktop_entry_reset(FcitxDesktopEntry *entry)
{
    utarray_done(&entry->comments);
    entry->flags = 0;
}

static void
fcitx_desktop_group_reset(FcitxDesktopGroup *group)
{
    utarray_done(&group->comments);
    FcitxDesktopEntry *entry;
    for (entry = group->first;entry;entry = entry->next)
        fcitx_desktop_entry_reset(entry);
    group->flags = 0;
}

static void
fcitx_desktop_file_reset(FcitxDesktopFile *file)
{
    utarray_done(&file->comments);
    FcitxDesktopGroup *group;
    for (group = file->first;group;group = group->next)
        fcitx_desktop_group_reset(group);
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
        switch (str[res - 1]) {
        case ' ':
        case '\r':
        case '\t':
        case '\f':
        case '\v':
            continue;
        }
        break;
    }
    return res;
}

static void
fcitx_desktop_group_remove_entry(FcitxDesktopGroup *group,
                                  FcitxDesktopEntry *entry)
{
    HASH_DEL(group->entries, entry);
    free(entry->name);
    fcitx_utils_free(entry->value);
    utarray_done(&entry->comments);
    if (entry->vtable && entry->vtable->free_entry) {
        entry->vtable->free_entry(entry->owner, entry);
    }
}

static void
fcitx_desktop_file_remove_group(FcitxDesktopFile *file,
                                FcitxDesktopGroup *group)
{
    FcitxDesktopEntry *entry;
    FcitxDesktopEntry *next;
    HASH_DEL(file->groups, group);
    for (entry = group->entries;entry;entry = next) {
        next = entry->hh.next;
        fcitx_desktop_group_remove_entry(group, entry);
    }
    free(group->name);
    utarray_done(&group->comments);
    if (group->vtable && group->vtable->free_group) {
        group->vtable->free_group(entry->owner, group);
    }
}

FCITX_EXPORT_API void
fcitx_desktop_file_done(FcitxDesktopFile *file)
{
    FcitxDesktopGroup *group;
    FcitxDesktopGroup *next;
    for (group = file->groups;group;group = next) {
        next = group->hh.next;
        fcitx_desktop_file_remove_group(file, group);
    }
    utarray_done(&file->comments);
}

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
            fcitx_desktop_group_remove_entry(group, entry);
        }
    }
}

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
            fcitx_desktop_file_remove_group(file, group);
        }
    }
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_file_find_group_with_len(FcitxDesktopFile *file,
                                       const char *name, size_t name_len)
{
    FcitxDesktopGroup *group = NULL;
    HASH_FIND(hh, file->groups, name, name_len, group);
    return group;
}

static FcitxDesktopGroup*
fcitx_desktop_file_hash_new_group(FcitxDesktopFile *file,
                                  const char *name, size_t name_len)
{
    FcitxDesktopGroup *new_group;
    new_group = fcitx_desktop_parse_new_group(file->vtable, file->owner);
    new_group->name = malloc(name_len + 1);
    memcpy(new_group->name, name, name_len);
    new_group->name[name_len] = '\0';
    HASH_ADD_KEYPTR(hh, file->groups, new_group->name, name_len, new_group);
    return new_group;
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_group_find_entry_with_len(FcitxDesktopGroup *group,
                                        const char *name, size_t name_len)
{
    FcitxDesktopEntry *entry = NULL;
    HASH_FIND(hh, group->entries, name, name_len, entry);
    return entry;
}

static FcitxDesktopEntry*
fcitx_desktop_group_hash_new_entry(FcitxDesktopGroup *group,
                                   const char *name, size_t name_len)
{
    FcitxDesktopEntry *new_entry;
    new_entry = fcitx_desktop_parse_new_entry(group->vtable, group->owner);
    new_entry->name = malloc(name_len + 1);
    memcpy(new_entry->name, name, name_len);
    new_entry->name[name_len] = '\0';
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
    UT_array comments;
    utarray_init(&comments, fcitx_str_icd);
    fcitx_desktop_file_reset(file);
    FcitxDesktopGroup *cur_group = NULL;

    while (getline(&buff, &buff_len, fp) != -1) {
        size_t line_len = strcspn(buff, "\n");
        buff[line_len] = '\0';
        lineno++;
        char *line = _find_none_blank(buff);
        switch (*line) {
        case '#':
            line++;
            utarray_push_back(&comments, &line);
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
                    FcitxLog(ERROR, _("Duplicate group %s in a desktop file,"
                                      "@ line %d, override previous one."),
                             new_group->name, lineno);
                    if (new_group == cur_group)
                        cur_group = new_group->prev;
                    fcitx_desktop_group_unlink(file, new_group);
                    fcitx_desktop_group_reset(new_group);
                }
                new_group->first = NULL;
                new_group->last = NULL;
            }
            new_group->flags |= FX_DESKTOP_GROUP_UPDATED;
            fcitx_desktop_file_link_group_after(file, cur_group, new_group);
            new_group->comments = comments;
            utarray_init(&comments, fcitx_str_icd);
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
        new_entry->comments = comments;
        utarray_init(&comments, fcitx_str_icd);
        char *value = line + key_len + 1;
        int value_len = buff + line_len - value;
        if (value_len <= 0) {
            fcitx_utils_free(new_entry->value);
            new_entry->value = NULL;
        } else {
            new_entry->value = realloc(new_entry->value, value_len + 1);
            memcpy(new_entry->value, value, value_len);
            new_entry->value[value_len] = '\0';
        }
    }
    file->comments = comments;
    fcitx_desktop_file_clean(file);
    return true;
}

boolean
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
fcitx_desktop_write_comments(FILE *fp, UT_array *comments)
{
    utarray_foreach(comment, comments, char*) {
        size_t len = _check_single_line(*comment);
        if (!len)
            continue;
        if (**comment == ' ' || **comment == '#') {
            _write_str(fp, "#");
        } else {
            _write_str(fp, "# ");
        }
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
fcitx_desktop_entry_write_fp(FcitxDesktopEntry *entry, FILE *fp)
{
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
fcitx_desktop_group_write_fp(FcitxDesktopGroup *group, FILE *fp)
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
fcitx_desktop_file_write_fp(FcitxDesktopFile *file, FILE *fp)
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

boolean
fcitx_desktop_file_write(FcitxDesktopFile *file, const char *name)
{
    boolean res;
    FILE *fp = fopen(name, "w");
    res = fcitx_desktop_file_write_fp(file, fp);
    if (fp)
        fclose(fp);
    return res;
}

static boolean
fcitx_desktop_file_has_group(FcitxDesktopFile *file, FcitxDesktopGroup *group)
{
    FcitxDesktopGroup *group_found;
    group_found = fcitx_desktop_file_find_group(file, group->name);
    return group_found == group;
}

FCITX_EXPORT_API FcitxDesktopGroup*
fcitx_desktop_file_add_group_after_with_len(
    FcitxDesktopFile *file, FcitxDesktopGroup *group,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopGroup *new_group;
    if (group) {
        if (!fcitx_desktop_file_has_group(file, group)) {
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
        utarray_init(&new_group->comments, fcitx_str_icd);
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
        if (!fcitx_desktop_file_has_group(file, group)) {
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
        utarray_init(&new_group->comments, fcitx_str_icd);
    } else if (move && !(new_group == group)) {
        fcitx_desktop_group_unlink(file, new_group);
        fcitx_desktop_file_link_group_before(file, group, new_group);
    }
    return new_group;
}

static boolean
fcitx_desktop_group_has_entry(FcitxDesktopGroup *group, FcitxDesktopEntry *entry)
{
    FcitxDesktopEntry *entry_found;
    entry_found = fcitx_desktop_group_find_entry(group, entry->name);
    return entry_found == entry;
}

FCITX_EXPORT_API FcitxDesktopEntry*
fcitx_desktop_group_add_entry_after_with_len(
    FcitxDesktopGroup *group, FcitxDesktopEntry *entry,
    const char *name, size_t name_len, boolean move)
{
    FcitxDesktopEntry *new_entry;
    if (entry) {
        if (!fcitx_desktop_group_has_entry(group, entry)) {
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
        utarray_init(&new_entry->comments, fcitx_str_icd);
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
        if (!fcitx_desktop_group_has_entry(group, entry)) {
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
        utarray_init(&new_entry->comments, fcitx_str_icd);
    } else if (move && !(new_entry == entry)) {
        fcitx_desktop_entry_unlink(group, new_entry);
        fcitx_desktop_group_link_entry_before(group, entry, new_entry);
    }
    return new_entry;
}
