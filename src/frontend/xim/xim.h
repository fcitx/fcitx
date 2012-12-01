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
    Window xim_window;
    UT_array* queue;
} FcitxXimFrontend;

CONFIG_BINDING_DECLARE(FcitxXimFrontend)
/*
 * `C' and `no' are additional one which cannot be obtained from modern
 * locale.gen. `no' is obsolete, but we keep it here for compatible reason.
 */
#define LOCALES_STRING \
    "aa,af,am,an,ar,as,ast,az,be,bem,ber,bg,bho,bn,bo,br,brx,bs,byn," \
    "C,ca,crh,cs,csb,cv,cy,da,de,dv,dz,el,en,es,et,eu,fa,ff,fi,fil,fo," \
    "fr,fur,fy,ga,gd,gez,gl,gu,gv,ha,he,hi,hne,hr,hsb,ht,hu,hy,id,ig," \
    "ik,is,it,iu,iw,ja,ka,kk,kl,km,kn,ko,kok,ks,ku,kw,ky,lb,lg,li,lij," \
    "lo,lt,lv,mag,mai,mg,mhr,mi,mk,ml,mn,mr,ms,mt,my,nan,nb,nds,ne,nl," \
    "nn,no,nr,nso,oc,om,or,os,pa,pap,pl,ps,pt,ro,ru,rw,sa,sc,sd,se,shs," \
    "si,sid,sk,sl,so,sq,sr,ss,st,sv,sw,ta,te,tg,th,ti,tig,tk,tl,tn,tr," \
    "ts,tt,ug,uk,unm,ur,uz,ve,vi,wa,wae,wal,wo,xh,yi,yo,yue,zh,zu"

#define LOCALES_BUFSIZE (sizeof(LOCALES_STRING) + 32)

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
