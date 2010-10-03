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
/**
 * @file   MainWindow.c
 * @author Yuking yuking_net@sohu.com
 * @date   2008-1-16
 * 
 * @brief  主窗口
 * 
 * 
 */

#include <stdio.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include "ui/ui.h"
#include "ui/skin.h"
#include "MainWindow.h"
#include "fcitx-config/profile.h"
#include "fcitx-config/configfile.h"
#include "fcitx-config/cutils.h"

MainWindow          mainWindow;

Bool		bMainWindow_Hiden = False;

extern Display *dpy;
extern Bool     bSP;
extern Bool     bVK;
extern CARD16   connect_id;

extern unsigned char iCurrentVK;
extern int iScreen;

static void InitMainWindow();

void InitMainWindow()
{
    memset(&mainWindow, 0, sizeof(MainWindow));
}

Bool CreateMainWindow (void)
{
	int depth;
	Colormap cmap;
	Visual * vs;
	XSetWindowAttributes attrib;
    unsigned long	attribmask;
	GC gc;
    char		strWindowName[]="Fcitx Main Window";
    
    InitMainWindow();

	XGCValues xgv;

	load_main_img();
	    
   	vs=find_argb_visual (dpy, iScreen);

    InitWindowAttribute(&vs, &cmap, &attrib, &attribmask, &depth);
	mainWindow.window=XCreateWindow (dpy, 
							  RootWindow(dpy, iScreen),
							  fcitxProfile.iMainWindowOffsetX,
							  fcitxProfile.iMainWindowOffsetY,
							  sc.skinMainBar.backImg.width, 
							  sc.skinMainBar.backImg.height,
							  0, depth,InputOutput, vs,attribmask, &attrib);

	if(mainWindow.window == None)
		return False;
		
	xgv.foreground = WhitePixel(dpy, iScreen);
	mainWindow.pm_main_bar = XCreatePixmap(
            dpy,
            mainWindow.window,
            sc.skinMainBar.backImg.width,
            sc.skinMainBar.backImg.height,
            depth);
	gc = XCreateGC(dpy,mainWindow.pm_main_bar, GCForeground, &xgv);
	XFillRectangle(dpy, mainWindow.pm_main_bar, gc, 0, 0,sc.skinMainBar.backImg.width, sc.skinMainBar.backImg.height);	
	mainWindow.cs_main_bar=cairo_xlib_surface_create(
            dpy,
            mainWindow.pm_main_bar,
            vs,
            sc.skinMainBar.backImg.width,
            sc.skinMainBar.backImg.height);	
    XFreeGC(dpy,gc);

	mainWindow.main_win_gc = XCreateGC( dpy, mainWindow.window, 0, NULL );
	XChangeWindowAttributes (dpy, mainWindow.window, attribmask, &attrib);	
	XSelectInput (dpy, mainWindow.window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask);

    XTextProperty	tp;
    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, mainWindow.window, &tp);
	
    return True;
}

void DisplayMainWindow (void)
{
#ifdef _DEBUG
    FcitxLog(DEBUG, _("DISPLAY MainWindow"));
#endif
	
    if (!bMainWindow_Hiden)
	XMapRaised (dpy, mainWindow.window);
}

void DrawMainWindow (void)
{
    INT8            iIndex = 0;
	cairo_t *c;
	Bool btmpPunc;

    if ( bMainWindow_Hiden )
    	return;
	
    iIndex = IS_CLOSED;

	//中英标点符号咋就反了?修正	
	btmpPunc=fcitxProfile.bChnPunc?False:True;
#ifdef _DEBUG
    FcitxLog(DEBUG, _("DRAW MainWindow"));
#endif
	//XResizeWindow(dpy, mainWindow, sc.skinMainBar.backImg.width, sc.skinMainBar.backImg.height);

	c=cairo_create(mainWindow.cs_main_bar);
	//把背景清空
	cairo_set_source_rgba(c, 0, 0, 0,0);
	cairo_rectangle (c, 0, 0, SIZEX, SIZEY);
	cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
	cairo_fill(c);
	
	cairo_set_operator(c, CAIRO_OPERATOR_OVER);

	if (fc.hideMainWindow == HM_SHOW || (fc.hideMainWindow == HM_AUTO && (ConnectIDGetState (connect_id) != IS_CLOSED)))
    {	
   // extern mouse_e ms_logo,ms_punc,ms_corner,ms_lx,ms_chs,ms_lock,ms_vk,ms_py;
		draw_a_img(&c, sc.skinMainBar.backImg,bar,RELEASE );
		draw_a_img(&c, sc.skinMainBar.logo,logo,ms_logo);
		draw_a_img(&c, sc.skinMainBar.zhpunc,punc[btmpPunc],ms_punc);
		draw_a_img(&c, sc.skinMainBar.chs,chs_t[fcitxProfile.bUseGBKT],ms_chs);
		draw_a_img(&c, sc.skinMainBar.halfcorner,corner[fcitxProfile.bCorner],ms_corner);
		draw_a_img(&c, sc.skinMainBar.unlock,lock[fcitxProfile.bLocked],ms_lock);
		draw_a_img(&c, sc.skinMainBar.nolegend,lx[fcitxProfile.bUseLegend],ms_lx);
		draw_a_img(&c, sc.skinMainBar.novk,vk[bVK],ms_vk);
		
		iIndex = ConnectIDGetState (connect_id);
		//printf("[iIndex:%d iIMIndex:%d imName:%s]\n",iIndex,iIMIndex,im[iIMIndex].strName);
		if( iIndex == 1 || iIndex ==0 )
		{
			//英文
			draw_a_img(&c, sc.skinMainBar.eng,english,ms_py);
		}
		else 
		{
			//默认码表显示
            if (im[gs.iIMIndex].icon)
                draw_a_img(&c, im[gs.iIMIndex].image, im[gs.iIMIndex].icon , ms_py);
            else
			{	//如果非默认码表的内容,则临时加载png文件.
				//暂时先不能自定义码表图片.其他码表统一用一种图片。
				//printf("[%s]\n",tmpstr);
				draw_a_img(&c, sc.skinMainBar.chn,otherim,ms_py);
			}
																		
		}
		XCopyArea (dpy, mainWindow.pm_main_bar, mainWindow.window, mainWindow.main_win_gc, 0, 0, sc.skinMainBar.backImg.width,\
		 															sc.skinMainBar.backImg.height, 0, 0);		
    }
    else
		XUnmapWindow (dpy, mainWindow.window);
	
	cairo_destroy(c);
}

void DestroyMainWindow()
{
    cairo_surface_destroy(mainWindow.cs_main_bar);
    XFreePixmap(dpy, mainWindow.pm_main_bar);
    XFreeGC(dpy, mainWindow.main_win_gc);
	XDestroyWindow(dpy, mainWindow.window);
}
