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

#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/skin.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"

void InitFont()
{
    if (!FcInit())
    {
        FcitxLog(FATAL, _("Error: Load fontconfig failed"));
        exit(1);
    }
}

void CreateFont()
{
    FcFontSet	*fs = NULL;
    FcPattern   *pat = NULL;
    FcObjectSet *os = NULL;

    char locale[3];

    if (fc.strUserLocale[0])
        strncpy(locale, fc.strUserLocale, 2);
    else
        strcpy(locale, "zh");
    locale[2]='\0';
reloadfont:
    if (strcmp(gs.fontZh, "") == 0)
    {
        FcChar8 strpat[9];
        sprintf((char*)strpat, ":lang=%s", locale);
        pat = FcNameParse(strpat);
    }
    else
    {
        pat = FcNameParse ((FcChar8*)gs.fontZh);
    }
    
    os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, (char*)0);
    fs = FcFontList(0, pat, os);
    if (os)
        FcObjectSetDestroy(os);
    os = NULL;
    
    FcPatternDestroy(pat);
    pat = NULL;

    if (!fs || fs->nfont <= 0)
        goto nofont;

    FcChar8* family;
    if (FcPatternGetString (fs->fonts[0], FC_FAMILY, 0, &family) != FcResultMatch)
        goto nofont;
    if (gs.fontZh)
        free(gs.fontZh);

    gs.fontZh = strdup((const char*) family);

    FcFontSetDestroy(fs);

    FcitxLog(INFO, _("your current font is: %s"), gs.fontZh);
    return;

nofont:
    if (strcmp(gs.fontZh, "") != 0)
    {
        strcpy(gs.fontZh, "");
        if (pat)
            FcPatternDestroy(pat);
        if (os)
            FcObjectSetDestroy(os);
        if (fs)
            FcFontSetDestroy(fs);

        goto reloadfont;
    }
        
    FcitxLog(FATAL, _("no valid font."));
    exit(1);
}
