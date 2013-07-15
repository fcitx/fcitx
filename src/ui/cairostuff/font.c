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

#include "config.h"

#ifndef _ENABLE_PANGO

#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/utils.h"
#include "fcitx-utils/log.h"

/**
 * Get Usable Font
 *
 * @param strUserLocale font language
 * @param font input as a malloc-ed font name, out put as new malloc-ed font name.
 * @return void
 **/
void GetValidFont(const char* strUserLocale, char **font)
{

    if (!FcInit()) {
        FcitxLog(ERROR, _("Error: Load fontconfig failed"));
        return;
    }
    char locale[3];

    if (strUserLocale)
        strncpy(locale, strUserLocale, 2);
    else
        strcpy(locale, "zh");
    locale[2] = '\0';

    boolean      found = false;
    while(true) {
        FcFontSet   *fs = NULL;
        FcPattern   *pat = NULL;
        FcObjectSet *os = NULL;

        do {
            if (strcmp(*font, "") == 0) {
                fcitx_utils_local_cat_str(strpat, strlen(":lang=") + sizeof(locale),
                                        ":lang=", locale);
                pat = FcNameParse((FcChar8*)strpat);
            } else {
                pat = FcNameParse((FcChar8*)(*font));
            }

            if (!pat) {
                break;
            }

            os = FcObjectSetBuild(FC_FAMILY, FC_STYLE, (char*)0);
            if (!os) {
                break;
            }

            fs = FcFontList(0, pat, os);
            if (!fs || fs->nfont <= 0) {
                break;
            }

            FcChar8* family;
            if (FcPatternGetString(fs->fonts[0], FC_FAMILY, 0, &family) != FcResultMatch) {
                break;
            }

            found = true;
            free(*font);
            *font = strdup((const char*) family);
        } while (0);

        if (fs) {
            FcFontSetDestroy(fs);
        }
        if (os) {
            FcObjectSetDestroy(os);
        }
        if (pat) {
            FcPatternDestroy(pat);
        }

        if (found) {
            break;
        }

        if (strcmp(*font, "") != 0) {
            /* *font must have enough space for "" */
            strcpy(*font, "");
        } else {
            break;
        }
    }

    if (found) {
        FcitxLog(INFO, _("your current font is: %s"), *font);
    } else {
        FcitxLog(WARNING, _("no valid font."));
    }
    return;
}
#endif

// kate: indent-mode cstyle; space-indent on; indent-width 0;
