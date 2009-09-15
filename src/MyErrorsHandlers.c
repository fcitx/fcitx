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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MyErrorsHandlers.h"

#include <stdio.h>
#include <signal.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_WAIT_H
#include <wait.h>
#else
#include <sys/wait.h>
#endif

#include "ime.h"
#include "tools.h"

#ifndef SIGUNUSED
#define SIGUNUSED 32
#endif

#ifdef _ENABLE_LOG
FILE *logfile;
#endif

XErrorHandler   oldXErrorHandler;

void SetMyExceptionHandler (void)
{
    int             signo;

    for (signo = SIGHUP; signo < SIGUNUSED; signo++)
	signal (signo, OnException);
}

void OnException (int signo)
{
    fprintf (stderr, "\nFCITX -- Get Signal No.: %d\n", signo);
    
#ifdef _ENABLE_LOG    
    struct tm  *ts;
    time_t now=time(NULL);
    char       buf[80];
    
    ts = localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    logfile=fopen("/var/log/fcitx.log", "a+t");     
    fprintf (logfile, "FCITX -- Get Signal No.: %d (%s)\n", signo,buf);
    fclose(logfile);
#endif
    
    if ( signo!=SIGSEGV && signo!=SIGCONT)
        SaveIM();
    
    switch (signo) {
    case SIGHUP:
	LoadConfig (False);
	SetIM ();
	break;
    case SIGINT:
    case SIGTERM:
    case SIGPIPE:
    case SIGSEGV:
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
    char            str[256];
    struct tm  *ts;
    time_t now=time(NULL);
    char       buf[80];
    
    XGetErrorText (dpy, event->error_code, str, 255);
    
    ts = localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S %Z", ts);
    logfile=fopen("/var/log/fcitx.log", "a+t");
    fprintf (logfile, "fcitx: %s\n", buf);
    fprintf (logfile, "fcitx: %s\n", str);
    fclose(logfile);
#endif

    if (event->error_code != 3 && event->error_code != BadMatch)	// xterm will generate 3
	oldXErrorHandler (dpy, event);

    return 0;
}
