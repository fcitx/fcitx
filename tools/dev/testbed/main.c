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

/**
 * @file fcitx.c
 * Main Function for fcitx
 */
#ifdef FCITX_HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <libintl.h>

#include "fcitx/instance.h"
#include "frontend/simple/simple.h"

FcitxInstance* instance = NULL;

int main(int argc, char* argv[])
{
    char* localedir = fcitx_utils_get_fcitx_path("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir);
    free(localedir);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");

    sem_t sem;
    sem_init(&sem, 0, 0);

    if (argc < 2) {
        return 1;
    }

    char* enableAddon = NULL;
    asprintf(&enableAddon, "fcitx-simple-module,fcitx-simple-frontend,%s", argv[1]);

    char* args[] = {
        argv[0],
        "-D",
        "--disable",
        "all",
        "--enable",
        enableAddon
    };

    if (argc < 3)
        setenv("XDG_CONFIG_HOME", argv[1], 1);
    else
        setenv("XDG_CONFIG_HOME", "/", 1);

    instance = FcitxInstanceCreatePause(&sem, 6, args, 0);
    free(enableAddon);

    FcitxSimpleGetFD(instance);
    FcitxSimpleGetSemaphore(instance);
    FcitxInstanceStart(instance);
    FcitxSimpleSendKeyEvent(instance, false, FcitxKey_a, 0, 0);
    char* candidateword = FcitxUICandidateWordToCString(instance);
    FcitxLog(INFO, "%s", candidateword);
    free(candidateword);
    return 0;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
