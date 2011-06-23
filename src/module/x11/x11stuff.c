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

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include "x11stuff.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include <unistd.h>

static void* X11Create(FcitxInstance* instance);
static void* X11Run();
static void* X11GetDisplay(void* x11priv, FcitxModuleFunctionArg arg);
static void* X11AddEventHandler(void* x11priv, FcitxModuleFunctionArg arg);
static void* X11RemoveEventHandler(void* x11priv, FcitxModuleFunctionArg arg);

typedef struct FcitxX11 {
    Display *dpy;
    UT_array handlers;
    FcitxInstance* owner;
} FcitxX11;

const UT_icd handler_icd = {sizeof(FcitxXEventHandler), 0, 0, 0};

FCITX_EXPORT_API
FcitxModule module = {
    X11Create,
    X11Run
};

void* X11Create(FcitxInstance* instance)
{
    FcitxX11* x11priv = fcitx_malloc0(sizeof(FcitxX11));
    FcitxAddon* x11addon = GetAddonByName(&instance->addons, FCITX_X11_NAME);
    XInitThreads();
    x11priv->dpy = XOpenDisplay(NULL);
    if (x11priv->dpy == NULL)
        return false;
    
    x11priv->owner = instance;

    utarray_init(&x11priv->handlers, &handler_icd);

    /* ensure the order ! */
    AddFunction(x11addon, X11GetDisplay);
    AddFunction(x11addon, X11AddEventHandler);
    AddFunction(x11addon,X11RemoveEventHandler);
    return x11priv;
}

void* X11Run(void* arg)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    XEvent event;
    /* 主循环，即XWindow的消息循环 */
    for (;;) {
        FcitxLock(x11priv->owner);
        while (XPending (x11priv->dpy) ) {
            XNextEvent (x11priv->dpy, &event);           //等待一个事件发生


            /* 处理X事件 */
            if (XFilterEvent (&event, None) == False)
            {
                FcitxXEventHandler* handler;
                for ( handler = (FcitxXEventHandler *) utarray_front(&x11priv->handlers);
                        handler != NULL;
                        handler = (FcitxXEventHandler *) utarray_next(&x11priv->handlers, handler))
                    if (handler->eventHandler (handler->instance, &event))
                        break;
            }
        }
        FcitxUnlock(x11priv->owner);
        usleep(16000);
    }
    return NULL;

}

void* X11GetDisplay(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    return x11priv->dpy;
}

void* X11AddEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    FcitxXEventHandler handler;
    handler.eventHandler = args.args[0];
    handler.instance = args.args[1];
    utarray_push_back(&x11priv->handlers, &handler);
    return NULL;
}

void* X11RemoveEventHandler(void* arg, FcitxModuleFunctionArg args)
{
    FcitxX11* x11priv = (FcitxX11*)arg;
    FcitxXEventHandler* handler;
    int i = 0;
    for ( i = 0 ;
            i < utarray_len(&x11priv->handlers);
            i ++)
    {
        handler = (FcitxXEventHandler*) utarray_eltptr(&x11priv->handlers, i);
        if (handler->instance == args.args[0])
            break;
    }
    utarray_erase(&x11priv->handlers, i, 1);
    return NULL;
}
