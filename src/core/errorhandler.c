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
#include "core/fcitx.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <limits.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_WAIT_H
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include <execinfo.h>

#include "core/ime.h"
#include "core/errorhandler.h"
#include "fcitx-config/xdg.h"
#include "utils/configfile.h"
#include "fcitx-config/cutils.h"
#include "core/ime-internal.h"

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif


void SetMyExceptionHandler (void)
{
    int             signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++)
    {
        signal (signo, OnException);
    }
}

void OnException (int signo)
{
    if (signo == SIGCHLD)
        return;

    FcitxLog(INFO, _("FCITX -- Get Signal No.: %d"), signo);

    if ( signo!=SIGSEGV && signo!=SIGCONT)
        SaveAllIM();

    void *array[10];
    size_t size;
    char **strings = NULL;
    size_t i;

    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);

    if (strings)
    {
        FILE *fp = NULL;
        if ( signo == SIGSEGV || signo == SIGABRT || signo == SIGKILL || signo == SIGTERM )
            fp = GetXDGFileUser("crash.log","wt", NULL);

        printf ("Obtained %zd stack frames.\n", size);
        if (fp)
        {
            fprintf (fp, "FCITX -- Get Signal No.: %d\n", signo);
            fprintf (fp, "Obtained %zd stack frames.\n", size);
        }

        for (i = 0; i < size; i++)
        {
            printf ("%s\n", strings[i]);
            if (fp)
                fprintf (fp, "%s\n", strings[i]);
        }

        if (fp)
            fclose(fp);
        free (strings);
    }

    switch (signo) {
    case SIGHUP:
        LoadConfig ();
        UnloadAllIM();
        LoadAllIM ();
        break;
    case SIGINT:
    case SIGTERM:
    case SIGPIPE:
    case SIGSEGV:
#ifdef _ENABLE_RECORDING
        CloseRecording();
#endif

        exit (0);
    default:
        break;
    }
}
