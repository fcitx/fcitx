/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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

#ifndef _FCITX_XIM_H_
#define _FCITX_XIM_H_

#include <X11/Xlib.h>
#include <X11/Xmd.h>

#include "fcitx/frontend.h"
#include "IMdkit.h"

#define DEFAULT_IMNAME "fcitx"
#define STRBUFLEN 64

#define GetXimIC(c) ((FcitxXimIC*)(c)->privateic)

typedef struct _FcitxXimFrontend {
    FcitxGenericConfig gconfig;
    boolean bUseOnTheSpotStyle;
    Window ximWindow;
    Display* display;
    int iScreen;
    int iTriggerKeyCount;
    XIMTriggerKey* Trigger_Keys;
    XIMS ims;
    CARD16 icid;
    struct _FcitxFrontend* frontend;
    struct _FcitxInstance* owner;
    int frontendid;
    CARD16 currentSerialNumberCallData;
    long unsigned int currentSerialNumberKey;
    XIMFeedback *feedback;
    int feedback_len;
} FcitxXimFrontend;

CONFIG_BINDING_DECLARE(FcitxXimFrontend)

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
