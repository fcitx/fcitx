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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _FCITX_PROFILE_H_
#define _FCITX_PROFILE_H_

#include "fcitx-config/fcitx-config.h"

typedef struct FcitxProfile
{
    GenericConfig gconfig;
    int iMainWindowOffsetX;
    int iMainWindowOffsetY;
    int iInputWindowOffsetX;
    int iInputWindowOffsetY;
    boolean bCorner;
    boolean bChnPunc;
    boolean bTrackCursor;
    boolean bUseLegend;
    int iIMIndex;
    boolean bUseGBKT;
    boolean bRecording;
} FcitxProfile;

void LoadProfile();
void SaveProfile();
boolean IsTrackCursor();
int GetInputWindowOffsetX();
int GetInputWindowOffsetY();
void SetInputWindowOffsetX(int pos);
void SetInputWindowOffsetY(int pos);

#endif
