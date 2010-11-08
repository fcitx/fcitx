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

#ifndef ICONV_CONST
#define ICONV_CONST
#endif

#ifndef FALSE
#define FALSE	0
#endif

typedef enum _IME_STATE {
    IS_CLOSED = 0,
    IS_ENG,
    IS_CHN
} IME_STATE;

Bool            InitXIM (char *);
void            SendHZtoClient (IMForwardEventStruct * call_data, char *strHZ);
void            EnterChineseMode (Bool bState);

void            SetIMState (Bool bState);
void		    SetTrackPos(IMChangeICStruct * call_data);
void            MyIMForwardEvent (CARD16 connectId, CARD16 icId, int keycode);

#define GetCurrentState() (CurrentIC?(CurrentIC->state):(IS_CLOSED))
#define GetCurrentPos() (CurrentIC?(&CurrentIC->pos):(NULL))

struct _IC;

extern struct _IC* CurrentIC;
void CloseAllIM();

#ifndef __USE_GNU
extern char    *strcasestr (__const char *__haystack, __const char *__needle);
#endif

#ifdef _ENABLE_RECORDING
Bool		OpenRecording(Bool);
void 		CloseRecording(void);
#endif

#endif
