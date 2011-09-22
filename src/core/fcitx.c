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
 * @file fcitx.c
 * @brief Main Function for fcitx
 */
#ifdef FCITX_HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <libintl.h>
#include <unistd.h>
#include <semaphore.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif // HAVE_UNISTD_H

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif // HAVE_MALLOC_H

#include "fcitx/configfile.h"
#include "fcitx/addon.h"
#include "fcitx/module.h"
#include "fcitx/ime-internal.h"
#include "fcitx/frontend.h"
#include "fcitx/profile.h"
#include "fcitx/instance.h"
#include "fcitx-utils/utils.h"
#include "errorhandler.h"

FcitxInstance* instance;

static void WaitForEnd(sem_t *sem, int count)
{
    while (count) {
        sem_wait(sem);
        count --;
    }
}

int main(int argc, char* argv[])
{
#ifdef M_TRIM_THRESHOLD
#ifdef HAVE_UNISTD_H
    int pagesize = sysconf(_SC_PAGESIZE);
#else
    int pagesize = 4 * 1024;
#endif // HAVE_UNISTD_H
    mallopt(M_TRIM_THRESHOLD, 5 * pagesize);
#endif // M_TRIM_THRESHOLD

    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", LOCALEDIR);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");
    SetMyExceptionHandler();

    int instanceCount = 1;

    sem_t sem;
    sem_init(&sem, 0, 0);

    instance = CreateFcitxInstance(&sem, argc, argv);

    WaitForEnd(&sem, instanceCount);
    return 0;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
