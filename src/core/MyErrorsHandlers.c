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
#include "core/MyErrorsHandlers.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

#ifdef _ENABLE_LOG
FILE *logfile = (FILE *)NULL;
#endif

XErrorHandler   oldXErrorHandler;

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
#ifdef _ENABLE_LOG
    struct tm	*ts;
    char	buf[PATH_MAX];
    time_t	now;

    if ( !logfile )
	logfile = GetXDGFileUser("fcitx.log","wt", NULL);
    if ( logfile ) {
	now=time(NULL);
	ts = localtime(&now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	fprintf (logfile, "%d: FCITX -- Get Signal No.: %d (%s)\n", getpid(), signo,buf);
    }
#endif

    if (signo == SIGCHLD)
        return;
    
    FcitxLog(INFO, _("FCITX -- Get Signal No.: %d"), signo);
    
    if ( signo!=SIGSEGV && signo!=SIGCONT)
        SaveIM();

    void *array[10];
    size_t size;
    char **strings = NULL;
    size_t i;
    
    size = backtrace (array, 10);
    strings = backtrace_symbols (array, size);
   
    if (strings)
    {
        printf ("Obtained %zd stack frames.\n", size);
        
        for (i = 0; i < size; i++)
            printf ("%s\n", strings[i]);
        
        free (strings);
    }
    
    switch (signo) {
    case SIGHUP:
	LoadConfig ();
    UnloadIM();
	SetIM ();
	break;
    case SIGINT:
    case SIGTERM:
    case SIGPIPE:
    case SIGSEGV:
#ifdef _ENABLE_LOG    
        if ( !logfile ) {
            fclose(logfile);
            logfile = (FILE *)NULL;
        }
#endif
#ifdef _ENABLE_RECORDING
        CloseRecording();
#endif

	exit (0);
    default:
	break;
    }
}

void SetMyXErrorHandler (void)
{
    oldXErrorHandler = XSetErrorHandler (MyXErrorHandler);
}

int MyXErrorHandler (Display * dpy, XErrorEvent * event)
{
#ifdef _ENABLE_LOG
    char	str[256];
    char	buf[PATH_MAX];
    struct tm	*ts;
    time_t	now;

    if ( !logfile )
        logfile = GetXDGFileUser("fcitx.log","wt" , NULL);
    if ( logfile ) {
        now = time(NULL);
	ts = localtime(&now);
	strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
	XGetErrorText (dpy, event->error_code, str, 255);
	fprintf (logfile, "fcitx: %s\n", buf);
	fprintf (logfile, "fcitx: %s\n", str);
    }
#endif

    SaveIM();

    if (event->error_code != 3 && event->error_code != BadMatch) {	// xterm will generate 3
#ifdef _ENABLE_LOG
	if ( !logfile ) {
            fclose(logfile);
            logfile = (FILE *)NULL;
        }
#endif	
	oldXErrorHandler (dpy, event);
    }

    return 0;
}
