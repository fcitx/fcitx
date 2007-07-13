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

typedef struct _CONNECT_ID {
    struct _CONNECT_ID	*next;
    CARD16	connect_id;
    IME_STATE	imState;
    Bool	bReset;
    //char	*strLocale;
} CONNECT_ID;

Bool            InitXIM (char *, Window);
void            SendHZtoClient (XIMS ims, IMForwardEventStruct * call_data, char *strHZ);
void		EnterChineseMode(void);
void            CreateConnectID(IMOpenStruct *call_data);
void            DestroyConnectID(CARD16 connect_id);
void            SetConnectID(CARD16 connect_id, IME_STATE	imState);
IME_STATE       ConnectIDGetState(CARD16 connect_id);
Bool		ConnectIDGetReset(CARD16 connect_id);
void		ConnectIDSetReset(CARD16 connect_id, Bool bReset);
void		ConnectIDResetReset(void);
//char           *ConnectIDGetLocale(CARD16 connect_id);

#endif
