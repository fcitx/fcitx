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
#include "clipboard-internal.h"
#include "fcitx/module.h"
#include "module/x11/x11stuff.h"

static void
_X11ClipboardPrimaryConvertCb(
    void *owner, const char *sel_str, const char *tgt_str, int format,
    size_t nitems, const void *buff, void *data)
{
    printf("%s, primary, %s, %s, %s\n", __func__, sel_str, tgt_str, (char*)buff);
}

static void
_X11ClipboardPrimaryNotifyCb(void *owner, const char *sel_str,
                             int subtype, void *data)
{
    FcitxClipboard *clipboard = owner;
    printf("%s, %s\n", __func__, sel_str);
    FcitxX11RequestConvertSelect(clipboard->owner, "PRIMARY", "UTF8_STRING",
                                 clipboard, _X11ClipboardPrimaryConvertCb,
                                 NULL, NULL);
}

static void
_X11ClipboardClipboardConvertCb(
    void *owner, const char *sel_str, const char *tgt_str, int format,
    size_t nitems, const void *buff, void *data)
{
    printf("%s, clipboard, %s, %s, %s\n", __func__, sel_str, tgt_str, (char*)buff);
}

static void
_X11ClipboardClipboardNotifyCb(void *owner, const char *sel_str,
                               int subtype, void *data)
{
    FcitxClipboard *clipboard = owner;
    printf("%s, %s\n", __func__, sel_str);
    FcitxX11RequestConvertSelect(clipboard->owner, "CLIPBOARD", "UTF8_STRING",
                                 clipboard, _X11ClipboardClipboardConvertCb,
                                 NULL, NULL);
}

void
ClipboardInitX11(FcitxClipboard *clipboard)
{
    FcitxInstance *instance = clipboard->owner;
    clipboard->x11 = FcitxX11GetAddon(instance);
    if (!clipboard->x11)
        return;
    clipboard->x11_primary_notify_id = FcitxX11RegSelectNotify(
        instance, "PRIMARY", clipboard,
        _X11ClipboardPrimaryNotifyCb, NULL, NULL);
    clipboard->x11_clipboard_notify_id = FcitxX11RegSelectNotify(
        instance, "CLIPBOARD", clipboard,
        _X11ClipboardClipboardNotifyCb, NULL, NULL);
}
