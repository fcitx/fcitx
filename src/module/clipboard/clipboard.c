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

#include "fcitx/fcitx.h"
#include "config.h"
#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx/context.h"
#include "fcitx/module.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"

#include "clipboard-internal.h"
#ifdef ENABLE_X11
#include "clipboard-x11.h"
#endif

CONFIG_DEFINE_LOAD_AND_SAVE(FcitxClipboard, FcitxClipboardConfig,
                            "fcitx-clipboard");

static void *ClipboardCreate(FcitxInstance *instance);
static void ClipboardDestroy(void *arg);
static void ClipboardReloadConfig(void *arg);
static void ApplyClipboardConfig(FcitxClipboard *clipboard);

FCITX_DEFINE_PLUGIN(fcitx_clipboard, module, FcitxModule) = {
    .Create = ClipboardCreate,
    .Destroy = ClipboardDestroy,
    .SetFD = NULL,
    .ProcessEvent = NULL,
    .ReloadConfig = ClipboardReloadConfig
};

static void*
ClipboardCreate(FcitxInstance *instance)
{
    FcitxClipboard *clipboard = fcitx_utils_new(FcitxClipboard);
    clipboard->owner = instance;

    if (!FcitxClipboardLoadConfig(&clipboard->config)) {
        ClipboardDestroy(clipboard);
        return NULL;
    }
    ClipboardInitX11(clipboard);
    ApplyClipboardConfig(clipboard);
    return clipboard;
}

static void
ClipboardDestroy(void *arg)
{
    FcitxClipboard *clipboard = (FcitxClipboard*)arg;
    fcitx_utils_free(clipboard->primary.str);
    free(arg);
}

static void
ApplyClipboardConfig(FcitxClipboard *clipboard)
{
    FcitxClipboardConfig *config = &clipboard->config;
    if (config->history_len < 1) {
        config->history_len = 1;
    } else if (config->history_len > CLIPBOARD_MAX_LEN) {
        config->history_len = CLIPBOARD_MAX_LEN;
    }
    while (clipboard->clp_hist_len > config->history_len) {
        char *str = clipboard->clp_hist_lst[--clipboard->clp_hist_len].str;
        fcitx_utils_free(str);
    }
}

static void
ClipboardReloadConfig(void* arg)
{
    FcitxClipboard *clipboard = (FcitxClipboard*)arg;
    FcitxClipboardLoadConfig(&clipboard->config);
    ApplyClipboardConfig(clipboard);
}

void
ClipboardSetPrimary(FcitxClipboard *clipboard, size_t len, const char *str)
{
    if (!(len && str))
        return;
    clipboard->primary.str = realloc(clipboard->primary.str, len + 1);
    memcpy(clipboard->primary.str, str, len);
    clipboard->primary.str[len] = '\0';
    clipboard->primary.len = len;
}

void
ClipboardPushClipboard(FcitxClipboard *clipboard, size_t len, const char *str)
{
    if (!(len && str))
        return;
    if (clipboard->clp_hist_len &&
        len == clipboard->clp_hist_lst->len &&
        !memcmp(clipboard->clp_hist_lst->str, str, len))
        return;
    char *new_str;
    if (clipboard->clp_hist_len < clipboard->config.history_len) {
        clipboard->clp_hist_len++;
        new_str = NULL;
    } else {
        new_str = clipboard->clp_hist_lst[clipboard->clp_hist_len - 1].str;
    }
    memmove(clipboard->clp_hist_lst + 1, clipboard->clp_hist_lst,
            (clipboard->clp_hist_len - 1) * sizeof(ClipboardSelectionStr));
    new_str = realloc(new_str, len + 1);
    memcpy(new_str, str, len);
    new_str[len] = '\0';
    clipboard->clp_hist_lst->len = len;
    clipboard->clp_hist_lst->str = new_str;
}
