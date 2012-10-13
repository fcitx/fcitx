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
fcitx_desktop_parse_new_group(FcitxDesktopVTable *vtable)
{
    FcitxDesktopGroup *group;
    if (vtable && vtable->new_group) {
        group = vtable->new_group(vtable->priv);
        memset(group, 0, sizeof(FcitxDesktopGroup));
    } else {
        group = fcitx_utils_new(FcitxDesktopGroup);
    }
    group->vtable = vtable;
    return group;
}

static FcitxDesktopEntry*
fcitx_desktop_parse_new_entry(FcitxDesktopVTable *vtable)
{
    FcitxDesktopEntry *entry;
    if (vtable && vtable->new_entry) {
        entry = vtable->new_entry(vtable->priv);
        memset(entry, 0, sizeof(FcitxDesktopEntry));
    } else {
        entry = fcitx_utils_new(FcitxDesktopEntry);
    }
    entry->vtable = vtable;
    return entry;
}

FCITX_EXPORT_API boolean
fcitx_desktop_file_init(FcitxDesktopFile *file, FcitxDesktopVTable *vtable)
{
    if (!fcitx_desktop_parse_check_vtable(vtable))
        return false;
    memset(file, 0, sizeof(FcitxDesktopFile));
    file->vtable = vtable;
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
fcitx_desktop_entry_free(FcitxDesktopEntry *entry)
{
    free(entry->name);
    fcitx_utils_free(entry->value);
    utarray_done(&entry->comments);
    if (entry->vtable && entry->vtable->free_entry) {
        entry->vtable->free_entry(entry->vtable->priv, entry);
    }
}

static void
fcitx_desktop_group_free(FcitxDesktopGroup *group)
{
    FcitxDesktopEntry *entry;
    FcitxDesktopEntry *next;
    for (entry = group->entries;entry;entry = next) {
        next = entry->hh.next;
        HASH_DEL(group->entries, entry);
        fcitx_desktop_entry_free(entry);
    }
    free(group->name);
    utarray_done(&group->comments);
    if (group->vtable && group->vtable->free_group) {
        group->vtable->free_group(entry->vtable->priv, group);
    }
}

FCITX_EXPORT_API void
fcitx_desktop_file_done(FcitxDesktopFile *file)
{
    FcitxDesktopGroup *group;
    FcitxDesktopGroup *next;
    for (group = file->groups;group;group = next) {
        next = group->hh.next;
        HASH_DEL(file->groups, group);
        fcitx_desktop_group_free(group);
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
            HASH_DEL(group->entries, entry);
            fcitx_desktop_entry_free(entry);
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
            HASH_DEL(file->groups, group);
            fcitx_desktop_group_free(group);
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
    new_group = fcitx_desktop_parse_new_group(file->vtable);
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
    new_entry = fcitx_desktop_parse_new_entry(group->vtable);
    new_entry->name = malloc(name_len + 1);
    memcpy(new_entry->name, name, name_len);
    new_entry->name[name_len] = '\0';
    HASH_ADD_KEYPTR(hh, group->entries, new_entry->name, name_len, new_entry);
    return new_entry;
}

FCITX_EXPORT_API void
fcitx_desktop_file_load_fp(FcitxDesktopFile *file, FILE *fp)
{
    if (!(file && fp))
        return;
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
            if (!new_group)
                new_group = fcitx_desktop_file_hash_new_group(file, line,
                                                              name_len);
            new_group->flags |= FX_DESKTOP_GROUP_UPDATED;
            new_group->first = NULL;
            new_group->last = NULL;
            if (!file->first)
                file->first = new_group;
            if (cur_group)
                cur_group->next = new_group;
            new_group->prev = cur_group;
            new_group->next = NULL;
            file->last = new_group;
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
        if (!new_entry)
            new_entry = fcitx_desktop_group_hash_new_entry(cur_group,
                                                           line, key_len);
        new_entry->flags |= FX_DESKTOP_ENTRY_UPDATED;
        if (!cur_group->first)
            cur_group->first = new_entry;
        FcitxDesktopEntry *prev = cur_group->last;
        if (prev)
            prev->next = new_entry;
        new_entry->prev = prev;
        new_entry->next = NULL;
        cur_group->last = new_entry;
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
        if (**comment == ' ') {
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

FCITX_EXPORT_API void
fcitx_desktop_file_write_fp(FcitxDesktopFile *file, FILE *fp)
{
    if (!(file && fp))
        return;
    FcitxDesktopGroup *group;
    for (group = file->first;group;group = group->next) {
        fcitx_desktop_group_write_fp(group, fp);
    }
    fcitx_desktop_write_comments(fp, &file->comments);
}
