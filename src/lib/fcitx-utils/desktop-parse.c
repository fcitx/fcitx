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

#define case_blank case ' ': case '\t': case '\b': case '\n': case '\f': \
case '\v': case '\r'

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
    for (;*str;str++) {
        switch (*str) {
        case_blank:
            continue;
        default:
            return str;
        }
    }
    return str;
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
    const char *end;
    for (end = str;*end;end++) {
        switch (*end) {
        case '[':
            return 0;
        case ']':
            return end - str;
        default:
            continue;
        }
    }
    return 0;
}

static size_t
fcitx_desktop_entry_key_len(const char *str)
{
    const char *end;
    for (end = str;*end;end++) {
        switch (*end) {
        case '=':
            return end - str;
        default:
            continue;
        }
    }
    return 0;
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
        size_t line_len = strlen(buff);
        if (buff[line_len - 1] == '\n') {
            line_len--;
            buff[line_len] = '\0';
        }
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
            FcitxDesktopGroup *new_group = NULL;
            HASH_FIND(hh, file->groups, line, name_len, new_group);
            if (!new_group) {
                new_group = fcitx_desktop_parse_new_group(file->vtable);
                new_group->name = malloc(name_len + 1);
                memcpy(new_group->name, line, name_len);
                new_group->name[name_len] = '\0';
                HASH_ADD_KEYPTR(hh, file->groups, new_group->name,
                                name_len, new_group);
            }
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
        FcitxDesktopEntry *new_entry = NULL;
        HASH_FIND(hh, cur_group->entries, line, key_len, new_entry);
        if (!new_entry) {
            new_entry = fcitx_desktop_parse_new_entry(file->vtable);
            new_entry->name = malloc(key_len + 1);
            memcpy(new_entry->name, line, key_len);
            new_entry->name[key_len] = '\0';
            HASH_ADD_KEYPTR(hh, cur_group->entries, new_entry->name,
                            key_len, new_entry);
        }
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

static inline void
_write_str(FILE *fp, char *str)
{
    if (str) {
        fwrite(str, strlen(str), 1, fp);
    }
}

static void
fcitx_desktop_write_comments(FILE *fp, UT_array *comments)
{
    utarray_foreach(comment, comments, char*) {
        size_t len = *comment ? strlen(*comment) : 0;
        if (!len)
            continue;
        if (**comment == ' ') {
            _write_str(fp, "#");
        } else {
            _write_str(fp, "# ");
        }
        fwrite(*comment, len, 1, fp);
        if ((*comment)[len - 1] != '\n') {
            _write_str(fp, "\n");
        }
    }
}

static void
fcitx_desktop_entry_write_fp(FcitxDesktopEntry *entry, FILE *fp)
{
    fcitx_desktop_write_comments(fp, &entry->comments);
    _write_str(fp, entry->name);
    _write_str(fp, "=");
    _write_str(fp, entry->value);
    _write_str(fp, "\n");
}

static void
fcitx_desktop_group_write_fp(FcitxDesktopGroup *group, FILE *fp)
{
    FcitxDesktopEntry *entry;
    fcitx_desktop_write_comments(fp, &group->comments);
    _write_str(fp, "[");
    _write_str(fp, group->name);
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
