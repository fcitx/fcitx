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
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>

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
#include "fcitx/instance-internal.h"
#include "fcitx-utils/utils.h"
#include "errorhandler.h"

FcitxInstance* instance = NULL;
int selfpipe[2];
char* crashlog = NULL;

int main(int argc, char* argv[])
{
    char* localedir = fcitx_utils_get_fcitx_path("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir);
    free(localedir);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");
    if (pipe(selfpipe) < 0) {
        fprintf(stderr, "Could not create self-pipe.\n");
        exit(1);
    }
    fcntl(selfpipe[0], F_SETFL, O_NONBLOCK);
    fcntl(selfpipe[1], F_SETFL, O_NONBLOCK);

    /* prepare filename first */
    FcitxXDGMakeDirUser("log");
    FcitxXDGGetFileUserWithPrefix("log", "crash.log", NULL, &crashlog);
    SetMyExceptionHandler();

    sem_t sem;
    sem_init(&sem, 0, 0);

    instance = FcitxInstanceCreateWithFD(&sem, argc, argv, selfpipe[0]);
    sem_wait(&sem);
    FcitxInstanceWaitForEnd(instance);

    free(crashlog);
    crashlog = NULL;
    if (instance->loadingFatalError) {
        return 1;
    }
    return 0;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
