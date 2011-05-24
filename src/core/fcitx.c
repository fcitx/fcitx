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

/**
 * @mainpage Fcitx
 * fcitx is a lightweight Input Method Framework, written by C.
 * It can be used under X11 to support international input.
 * 
 */

/**
 * @file fcitx.c
 * @brief Main Function for fcitx
 */

#include <locale.h>
#include <libintl.h>
#include <unistd.h>

#include "fcitx-utils/configfile.h"
#include "fcitx/addon.h"
#include "fcitx-utils/utils.h"
#include "fcitx/module.h"
#include "fcitx/ime-internal.h"
#include "fcitx/backend.h"
#include "fcitx-utils/profile.h"
#include <fcitx/instance.h>

static void WaitForEnd()
{
    while (true)
        sleep(10);
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);
    
    CreateFcitxInstance();
    
    WaitForEnd();
	return 0;
}