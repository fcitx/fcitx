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

#include "core/fcitx.h"
#include "core/module.h"
#include "x11stuff.h"
#include "utils/utils.h"
#include <unistd.h>

static boolean X11Init();
static void* X11Run();
static void* X11GetDisplay(FcitxModuleFunctionArg arg);
static void* X11AddEventHandler(FcitxModuleFunctionArg arg);
static void* X11RemoveEventHandler(FcitxModuleFunctionArg arg);

typedef struct FcitxX11 {
    Display *dpy;
    UT_array handlers;
} FcitxX11;

const UT_icd handler_icd = {sizeof(FcitxXEventHandler), 0, 0, 0};

FcitxX11 x11priv;

FCITX_EXPORT_API
FcitxModule module = {
    X11Init,
    X11Run
};

boolean X11Init()
{
    XInitThreads();
    x11priv.dpy = XOpenDisplay(NULL);
    if (x11priv.dpy == NULL)
        return false;

    utarray_init(&x11priv.handlers, &handler_icd);

    /* ensure the order ! */
    AddFunction(X11GetDisplay);
    AddFunction(X11AddEventHandler);
    AddFunction(X11RemoveEventHandler);

    return true;
}

void* X11Run()
{
    XEvent event;
    /* 主循环，即XWindow的消息循环 */
    for (;;) {
        while (XPending (x11priv.dpy) ) {
            XNextEvent (x11priv.dpy, &event);           //等待一个事件发生

            FcitxLock();

            /* 处理X事件 */
            if (XFilterEvent (&event, None) == False)
            {
                FcitxXEventHandler* handler;
                for ( handler = (FcitxXEventHandler *) utarray_front(&x11priv.handlers);
                        handler != NULL;
                        handler = (FcitxXEventHandler *) utarray_next(&x11priv.handlers, handler))
                    if (handler->eventHandler (handler->instance, &event))
                        break;
            }
            FcitxUnlock();
        }
        usleep(16000);
    }
    return NULL;

}

void* X11GetDisplay(FcitxModuleFunctionArg arg)
{
    return x11priv.dpy;
}

void* X11AddEventHandler(FcitxModuleFunctionArg arg)
{
    FcitxXEventHandler handler;
    handler.eventHandler = arg.args[0];
    handler.instance = arg.args[1];
    utarray_push_back(&x11priv.handlers, &handler);
    return NULL;
}

void* X11RemoveEventHandler(FcitxModuleFunctionArg arg)
{
    FcitxXEventHandler* handler;
    int i = 0;
    for ( i = 0 ;
            i < utarray_len(&x11priv.handlers);
            i ++)
    {
        handler = (FcitxXEventHandler*) utarray_eltptr(&x11priv.handlers, i);
        if (handler->instance == arg.args[0])
            break;
    }
    utarray_erase(&x11priv.handlers, i, 1);
    return NULL;
}
