/***************************************************************************
 *   小企鹅中文输入法(Free Chinese Input Toys for X, FCITX)                *
 *   由Yuking(yuking_net@sohu.com)编写                                     *
 *   协议: GPL                                                             *
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
#include <libintl.h>

#include <config.h>

#include "version.h"
#include "tools/tools.h"

#include "core/MyErrorsHandlers.h"
#include "core/ime.h"
#include "core/addon.h"
#include "ui/ui.h"
#include "ui/MainWindow.h"
#include "ui/InputWindow.h"
#include "ui/skin.h"
#include "ui/MenuWindow.h"
#include "ui/font.h"

#ifdef _ENABLE_TRAY
#include "ui/TrayWindow.h"
#endif

#include "interface/DBus.h"

#include "im/special/vk.h"
#include "core/ime.h"
#include "im/table/table.h"
#include "im/special/punc.h"
#include "im/pinyin/py.h"
#include "im/pinyin/sp.h"
#include "ui/AboutWindow.h"
#include "ui/MessageWindow.h"
#include "im/special/QuickPhrase.h"
#include "im/special/AutoEng.h"

#include "fcitx-config/profile.h"

#ifndef CODESET
#define CODESET 14
#endif
#include <pthread.h>

extern Display *dpy;
extern Window   ximWindow;
extern int iClientCursorX;
extern int iClientCursorY;

extern void* remoteThread(void*);

static void Usage();
static void Version();
static void InitGlobal();

/** 
 * @brief 初始化全局状态 
 */
void InitGlobal()
{
    memset(&gs, 0, sizeof(FcitxState));
}

/** 
 * @brief 主程序入口
 * 
 * @param argc 命令行参数个数
 * @param argv[] 命令行参数
 * 
 * @return 
 */
int main (int argc, char *argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR); 
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

    XEvent          event;
    int             c; 	/* 用于保存用户输入的参数 */
    Bool            bBackground = True;
    char	    *imname=(char *)NULL;
    pthread_t	    pid;

    InitGlobal();

    SetMyExceptionHandler();		/* 处理信号 */

    /*
     * 先初始化 X 再加载配置文件，因为设置快捷键从 keysym 转换到
     * keycode 的时候需要 Display
     */
    if (!InitX ())
	exit (1);

    /*
     * 加载用户配置文件
     */
    LoadConfig ();

    while((c = getopt(argc, argv, "cdDn:vh")) != -1) {
        switch(c){
            case 'd':
                /* nothing to do */
                break;
            case 'D':
                bBackground = False;
                break;
            case 'c':
                SaveConfig();
                return 0;
            case 'n':
                imname=optarg;
                break;
            case 'v':	/* 输出版本号 */
                Version();
                return 0;
            case 'h':	/* h或者其他任何不合法的参数均，输出参数帮助信息 */
            case '?':
                Usage();
                return 0;
        }
    }

#ifdef _ENABLE_DBUS
    /*
     * 启用DBus时初始化DBus
     */
    if (fc.bUseDBus && !InitDBus ())
	exit (5);
#endif
	
	/*
     * 加载皮肤配置文件,一般在share/fcixt/skin/skinname dir/fcitx_skin.conf中,制作皮肤的时候配置好
     */
	LoadSkinConfig();

    InitFont();

    /* 加载皮肤和配置之后才有字体 */
    CreateFont();

    /* 根据字体计算输入窗口的高度 */
    CalculateInputWindowHeight ();

    /*
     * 加载配置文件，这个配置文件不是用户配置的，而是用于记录fctix的运行状态的
     * 比如是全角还是半角等等。
     */
    LoadProfile ();

    LoadAddonInfo();

    iClientCursorX = fcitxProfile.iInputWindowOffsetX;
    iClientCursorY = fcitxProfile.iInputWindowOffsetY;

    /* 加载标点字典文件 */
    LoadPuncDict ();
    /* 加载快速词组 */
    LoadQuickPhrase ();
    /*
     * 从用户配置目录中读取AutoEng.dat
     * 如果不存在，则从 DATADIR/fcitx/data/AutoEng.dat）
     * 读取需要自动转换到英文输入状态的情况的数据
     */
    LoadAutoEng ();

    /* 以下是界面的处理 */
    /* 创建主窗口，即输入法状态窗口 */
    if (!fc.bUseDBus)
        CreateMainWindow ();
#ifdef _ENABLE_DBUS
    else
        registerProperties();
#endif

    /* 创建输入窗口 */
    CreateInputWindow ();

    /* 创建虚拟键盘窗口 */
    CreateVKWindow ();
    
    /* 创建菜单窗口 */
    CreateMenuWindow( ); 

	/* 创建信息提示窗口 */
	CreateMessageWindow();

    /* 创建关于窗口 */
    if (!fc.bUseDBus)
        CreateAboutWindow ();

    /* 将本程序加入到输入法组，告诉系统，使用我输入字符 */
    SetIM ();

    if (!fc.bUseDBus) {
        /* 处理主窗口的显示 */
        if (fc.hideMainWindow != HM_HIDE) {
            DisplayMainWindow ();
            DrawMainWindow ();
        }
    }

    /* 初始化XIM */
    if (!InitXIM (imname))
	exit (4);

    /* 以后台方式运行 */
    if (bBackground)
        InitAsDaemon();

#ifdef _ENABLE_RECORDING
    OpenRecording(True);
#endif

#ifdef _ENABLE_DBUS
    dbus_threads_init_default();
#endif
	
    FcitxInitThread();

    pthread_create(&pid, NULL, remoteThread, NULL);

#ifdef _ENABLE_DBUS
    if (fc.bUseDBus)
        pthread_create(&pid, NULL, (void *)DBusLoop, NULL);
#endif

#ifdef _ENABLE_TRAY
    tray.window = (Window) NULL;

    /* 创建系统托盘窗口 */
    if (!fc.bUseDBus) {
        CreateTrayWindow ();
    	DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size);
    }
#endif

    DisplaySkin(fc.skinType);

    /* 主循环，即XWindow的消息循环 */
    for (;;) {
        XNextEvent (dpy, &event);			//等待一个事件发生
        
        FcitxLock();
        
        /* 处理X事件 */
        if (XFilterEvent (&event, None) == False)
            MyXEventHandler (&event);
        
        FcitxUnlock();
    }


    return 0;
}

/** 
 * @brief 显示命令行参数
 */
void Usage ()
{
    printf("Usage: fcitx [OPTION]\n"
           "\t-d\t\trun as daemon(default)\n"
           "\t-D\t\tdon't run as daemon\n"
		   "\t-c\t\t(re)create config file in home directory and then exit\n"
		   "\t-n[im name]\trun as specified name\n"
           "\t-v\t\tdisplay the version information and exit\n"
           "\t-h\t\tdisplay this help and exit\n");
}

/** 
 * @brief 显示版本
 */
void Version ()
{
    printf ("fcitx version: %s\n", FCITX_VERSION);
}
