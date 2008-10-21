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
/**
 * @file   main.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 * 
 * @brief  程序主入口
 * 
 * 
 */

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
#include "AutoEng.h"

#ifndef CODESET
#define CODESET 14
#endif

extern Display *dpy;
extern Window   inputWindow;

extern Bool     bIsUtf8;

extern HIDE_MAINWINDOW hideMainWindow;

int main (int argc, char *argv[])
{
    XEvent          event;
    int             c;
    Bool            bBackground = True;

    while((c = getopt(argc, argv, "dDvh")) != -1) {
        switch(c){
            case 'd':
                /* nothing to do */
                break;
            case 'D':
                bBackground = False;
                break;
            case 'v':
                Version();
                return 0;
            case 'h':
            case '?':
                Usage();
                return 0;
        }
    }
 
    setlocale (LC_CTYPE, "");
    bIsUtf8 = (strcmp (nl_langinfo (CODESET), "UTF-8") == 0);

    /* 先初始化 X 再加载配置文件，因为设置快捷键从 keysym 转换到
     * keycode 的时候需要 Display
     */
    if (!InitX ())
	exit (1);

    LoadConfig (True);

    CreateFont ();
    CalculateInputWindowHeight ();
    LoadProfile ();

    LoadPuncDict ();
    LoadQuickPhrase ();
    LoadAutoEng ();

    CreateMainWindow ();
    CreateVKWindow ();
    CreateInputWindow ();
    CreateAboutWindow ();

    InitGC (inputWindow);

    SetIM ();

    if (hideMainWindow != HM_HIDE) {
	DisplayMainWindow ();
	DrawMainWindow ();
    }

    if (!InitXIM (inputWindow))
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
    printf("Usage: fcitx [OPTION]\n"
           "\t-d\trun as daemon(default)\n"
           "\t-D\tdon't run as daemon\n"
           "\t-v\tdisplay the version information and exit\n"
           "\t-h\tdisplay this help and exit\n");
}

void Version ()
{
    printf ("fcitx version: %s-%s\n", FCITX_VERSION, USE_XFT);
}
