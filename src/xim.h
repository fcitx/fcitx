#ifndef _xim_h
#define _xim_h

#include <stdio.h>
#include <X11/Xlocale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "IMdkit.h"
#include "Xi18n.h"

#define DEFAULT_IMNAME "fcitx"
#define STRBUFLEN 64

typedef enum _IME_STATE {
    IS_CLOSED = 0,
    IS_ENG,
    IS_CHN
} IME_STATE;

void            MyXEventHandler (Window, XEvent *);
Bool            InitXIM (char *, Window);
void            SendHZtoClient (XIMS ims, IMForwardEventStruct * call_data, char *strHZ);
Bool            IsKey (XIMS ims, IMForwardEventStruct * call_data, XIMTriggerKey * trigger);

#endif
