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

#include <locale.h>
#include <libintl.h>
#include <unistd.h>

#include "utils/configfile.h"
#include "addon.h"
#include "utils/utils.h"
#include "module.h"
#include "ime-internal.h"
#include "backend.h"
#include "utils/profile.h"

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

    LoadConfig();
    LoadProfile();
    LoadAddonInfo();
    AddonResolveDependency();
    
    InitBuiltInHotkey();
    
    FcitxInitThread();
//    InitAsDaemon();
    LoadModule();
    LoadAllIM();
    LoadUserInterface();
    StartBackend();
    WaitForEnd();
	return 0;
}