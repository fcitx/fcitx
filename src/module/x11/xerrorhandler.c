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

#include <X11/Xlib.h>
#include "fcitx/instance.h"
#include "fcitx-config/xdg.h"
#include "fcitx/ime.h"
#include <setjmp.h>
#include "xerrorhandler.h"

static XErrorHandler   oldXErrorHandler;
static XIOErrorHandler oldXIOErrorHandler;
static FcitxX11* x11handle;

static int FcitxXErrorHandler(Display * dpy, XErrorEvent * event);
static int FcitxXIOErrorHandler(Display *d);

void InitXErrorHandler(FcitxX11* x11priv)
{
    x11handle = x11priv;
    oldXErrorHandler = XSetErrorHandler(FcitxXErrorHandler);
    oldXIOErrorHandler = XSetIOErrorHandler(FcitxXIOErrorHandler);
}

void UnsetXErrorHandler(FcitxX11* x11priv)
{
    FCITX_UNUSED(x11priv);
    x11handle = NULL;
    /* we don't set back to old handler, to avoid any x error  */
}

extern jmp_buf FcitxRecover;

int FcitxXIOErrorHandler(Display *d)
{
    FCITX_UNUSED(d);
    if (!x11handle)
        return 0;

    if (FcitxInstanceGetIsDestroying(x11handle->owner))
        return 0;

    /* Do not log, because this is likely to happen while log out */
    FcitxInstanceSaveAllIM(x11handle->owner);

    FcitxInstanceEnd(x11handle->owner);

    longjmp(FcitxRecover, 1);
    return 0;
}

int FcitxXErrorHandler(Display * dpy, XErrorEvent * event)
{
    if (!x11handle)
        return 0;

    if (FcitxInstanceGetIsDestroying(x11handle->owner))
        return 0;

    char    str[256];
    FILE* fp = NULL;

    fp = FcitxXDGGetFileUserWithPrefix("log", "crash.log", "w" , NULL);
    if (fp) {
        XGetErrorText(dpy, event->error_code, str, 255);
        fprintf(fp, "fcitx: %s\n", str);
    }

    FcitxInstanceSaveAllIM(x11handle->owner);

    if (fp)
        fclose(fp);
    if (event->error_code != 3 && event->error_code != BadMatch) {
        // xterm will generate 3
        FcitxInstanceEnd(x11handle->owner);
    }

    return 0;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
