/***************************************************************************
 *   小企鹅中文输入法(Free Chinese Input Toys for X, FCITX)                   *
 *   由Yuking(yuking_net@sohu.com)编写                                           *
 *   协议: GPL                                                              *
 *   FCITX( A Chinese XIM Input Method) by Yuking (yuking_net@sohu.com)    *   
 *                                                                         *
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
#include <langinfo.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "version.h"

#include "main.h"
#include "MyErrorsHandlers.h"
#include "tools.h"
#include "ui.h"
#include "MainWindow.h"
#include "InputWindow.h"
#include "vk.h"
#include "ime.h"
#include "table.h"
#include "punc.h"
#include "py.h"
#include "sp.h"
#include "about.h"
#include "QuickPhrase.h"

#ifndef CODESET
#define CODESET 14
#endif

extern char strUserLocale[];

extern Display *dpy;
extern Window   inputWindow;
extern Window   mainWindow;
extern Window   aboutWindow;

extern Bool     bIsUtf8;

extern HIDE_MAINWINDOW hideMainWindow;

int main (int argc, char *argv[])
{
    XEvent          event;
    char           *imname = NULL;
    int             i;
    Bool     bBackground=True;

    for (i = 1; i < argc; i++) {
	if (!strcmp (argv[i], "-name"))
	    imname = argv[++i];
	else if (!strcmp (argv[i], "-nb"))
	    bBackground = False;
	else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "-?")) {
	    Usage ();
	    return 0;
	}
	else if (!strcmp (argv[i], "-v")) {
	    Version ();
	    return 0;
	}
    }

    setlocale (LC_CTYPE, "");
    bIsUtf8 = (strcmp (nl_langinfo (CODESET), "UTF-8") == 0);

    LoadConfig (True);
    if (!InitX ())
	exit (1);

    CreateFont ();
    CalculateInputWindowHeight ();
    LoadProfile ();

    LoadPuncDict ();
    LoadQuickPhrase ();

    CreateMainWindow ();
    InitGC (mainWindow);
    CreateVKWindow ();
    CreateInputWindow ();
    CreateAboutWindow ();

    SetIM ();
    
    if (!InitXIM (imname, inputWindow))
	exit (4);

    //以后台方式运行
    if (bBackground) {
	pid_t           id;

	id = fork ();
	if (id == -1) {
	    printf ("Can't run as a daemon！\n");
	    exit (1);
	}
	else if (id > 0)
	    exit (0);
    }

    SetMyExceptionHandler ();
    for (;;) {
	XNextEvent (dpy, &event);

	if (XFilterEvent (&event, None) == True)
	    continue;

	MyXEventHandler (&event);
    }

    return 0;
}

void Usage ()
{
    printf ("fcitx usage:\n -name imename: \t specify the imename\n -nb :\t\t\t run as foreground \n -v:\t\t\t display the version information and exit.\n -h:\t\t\t display this help page and exit\n");
}

void Version ()
{
    printf ("fcitx version: %s-%s\n", FCITX_VERSION,USE_XFT);
}
