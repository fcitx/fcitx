/***************************************************************************
 *   Copyright (C) 2009-2010 by t3swing                                    *
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
 * @file   skin.h
 * @author t3swing  t3swing@sina.com
 *        
 * @date   2009-10-9
 * 
 * @brief  皮肤设置相关定义及初始化加载工作
 * 
 * 
 */

#ifndef _SKIN_H
#define _SKIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/extensions/Xrender.h>
#include <dirent.h>
#include <sys/stat.h>
#include "MainWindow.h"
#include "InputWindow.h"
#include "core/IC.h"
#include "core/ime.h"
#include "tools/utarray.h"
#include "ui/MenuWindow.h"

#define SIZEX 800
#define SIZEY 200
//输入条最大长度(缓冲区大小由这个决定)
#define INPUT_BAR_MAX_LEN 1500

typedef enum
{
    R_COPY = 0,
    R_RESIZE = 1,
    R_FIX = 2
} RESIZERULE;

typedef struct 
{
	char *skinName;
	char *skinVersion;
	char *skinAuthor;
	char *skinDesc;
} SkinInfo;

typedef struct 
{
	int fontSize;
    ConfigColor fontColor[7];
    ConfigColor menuFontColor[2];
} SkinFont;

typedef struct
{
    FcitxImage backImg;
    RESIZERULE resize;
	int marginTop;
	int marginBottom;
    int marginLeft;
    int marginRight;
    ConfigColor activeColor;
    ConfigColor lineColor;
} SkinMenu;

typedef struct 
{
	FcitxImage backImg;
	FcitxImage logo;
	FcitxImage zhpunc;
	FcitxImage enpunc;
	FcitxImage chs;
	FcitxImage cht;
	FcitxImage halfcorner;
	FcitxImage fullcorner;
	FcitxImage unlock;
	FcitxImage lock;
	FcitxImage legend;
	FcitxImage nolegend;
	FcitxImage vk;
	FcitxImage novk;
	FcitxImage eng;
	FcitxImage chn;
} SkinMainBar;

typedef struct 
{	
	FcitxImage backImg;
	RESIZERULE resize;
	int	resizePos;
	int resizeWidth;
	int inputPos;
	int outputPos;
	int layoutLeft;
	int layoutRight;
	ConfigColor cursorColor;
	FcitxImage backArrow;
	FcitxImage forwardArrow;
} SkinInputBar;


typedef struct 
{
	FcitxImage active;
	FcitxImage inactive;
} SkinTrayIcon;

/** 
* 配置文件结构,方便处理,结构固定
*/
typedef struct 
{
    GenericConfig config;
    SkinInfo skinInfo;
    SkinFont skinFont;
    SkinMainBar skinMainBar;
    SkinInputBar skinInputBar;
    SkinTrayIcon skinTrayIcon;	
    SkinMenu skinMenu;
} FcitxSkin;

/**
*
*/
typedef struct
{
	char *dirbase[64];
} SkinDir;

//extern cairo_surface_t *cs_main_menu;
//extern cairo_surface_t *cs_vk_menu;
//extern cairo_surface_t *cs_im_menu;
//extern cairo_surface_t *cs_skin_menu;

extern cairo_surface_t *  bar;
extern cairo_surface_t *  logo;
extern cairo_surface_t *  punc[2];
extern cairo_surface_t *  corner[2];	
extern cairo_surface_t *  lx[2];	
extern cairo_surface_t *  chs_t[2];	
extern cairo_surface_t *  lock[2];	
extern cairo_surface_t *  vk[2];
extern cairo_surface_t *  input;
extern cairo_surface_t *  prev;
extern cairo_surface_t *  next;
extern cairo_surface_t *  english;
extern cairo_surface_t *  pinyin;
extern cairo_surface_t *  shuangpin;
extern cairo_surface_t *  quwei;
extern cairo_surface_t *  wubi;
extern cairo_surface_t *  mix;
extern cairo_surface_t *  erbi;
extern cairo_surface_t *  cangji;
extern cairo_surface_t *  wanfeng;
extern cairo_surface_t *  bingchan;
extern cairo_surface_t *  ziran;
extern cairo_surface_t *  dianbao;
extern cairo_surface_t *  otherim;
extern cairo_surface_t *  trayActive;
extern cairo_surface_t *  trayInactive;

extern MouseE ms_logo,ms_punc,ms_corner,ms_lx,ms_chs,ms_lock,ms_vk,ms_py;

extern Display *dpy;

extern INT8	iIMCount;
extern IM	*im;
extern FcitxSkin sc;
extern UT_array *skinBuf;
/**
*解析skin_xpm结构的值
*/
extern int str2skin_img(char * str,FcitxImage * img);
extern Visual * find_argb_visual (Display *dpy, int scr);
extern void load_main_img();
void load_tray_img();
void load_menu_img();
void load_input_img();
void load_input_msg();
void draw_a_img(cairo_t **c,FcitxImage img,cairo_surface_t * png,MouseE mouse);
void destroy_a_img(cairo_surface_t ** png);
extern void draw_input_bar(Messages * msgup, Messages *msgdown ,unsigned int * iwidth);
extern void draw_menu_background(xlibMenu * menu);
extern void set_mouse_status(MouseE m);
/**
* 加载皮肤配置文件
*/
int LoadSkinConfig();

/*
**
*/
int WritePrivateProfileString(char *SelectName, char *KeyName, char *String, char *FileName);
int GetPrivateProfileString(FILE * fptmp,char *SelectName, char *KeyName,char *DefaultString, char *ReturnString,int Size);
int fill_skin_config_int(FILE * fptmp,char * SelectName,char * KeyName);
int fill_skin_config_str(FILE * fptmp,char * SelectName,char * KeyName,char * str);

void DisplayInputBar(int barlen);
void DisplaySkin(char * skinname);
int loadSkinDir();

extern Bool IsInBox (int x0, int y0, int x1, int y1, int x2, int y2);
extern Bool IsInRspArea(int x0,int y0,FcitxImage img);

extern Visual * find_argb_visual (Display *dpy, int scr);

extern void SelectIM(int imidx);
extern void SelectVK(int );

#define fcitx_cairo_set_color(c, color) cairo_set_source_rgb((c), (color)->r, (color)->g, (color)->b)


CONFIG_BINDING_DECLARE(FcitxSkin);
#endif


