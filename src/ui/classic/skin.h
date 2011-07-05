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

#define SIZEX 800
#define SIZEY 200
#include "fcitx-config/uthash.h"
#include <cairo.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx/ui.h"

struct XlibMenu;
struct InputWindow;
struct Messages;
struct FcitxClassicUI;

typedef struct SkinImage
{
    char *name;
    cairo_surface_t *image;
    UT_hash_handle hh;
} SkinImage;

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
    int menuFontSize;
    ConfigColor fontColor[7];
    ConfigColor menuFontColor[2];
} SkinFont;

typedef struct
{
    char* backImg;
    int marginTop;
    int marginBottom;
    int marginLeft;
    int marginRight;
    ConfigColor activeColor;
    ConfigColor lineColor;
} SkinMenu;

/**
 * @brief The Main Window Skin description
 **/
typedef struct 
{
    char* backImg;
    char* logo;
    char* eng;
    char* active;
    int marginTop;
    int marginBottom;
    int marginLeft;
    int marginRight;
    char *placement;
    UT_array skinPlacement;
} SkinMainBar;

typedef struct 
{    
    char* backImg;
    ConfigColor cursorColor;
    int marginTop;
    int marginBottom;
    int marginLeft;
    int marginRight;
    char* backArrow;
    char* forwardArrow;
    int iBackArrowX;
    int iBackArrowY;
    int iForwardArrowX;
    int iForwardArrowY;
    int iInputPos;
    int iOutputPos;
} SkinInputBar;

typedef struct SkinPlacement
{
    char name[MAX_STATUS_NAME + 1];
    int x;
    int y;
    UT_hash_handle hh;
} SkinPlacement;

/**
 * @brief Tray Icon Image
 **/
typedef struct 
{
    /**
     * @brief Active Tray Icon Image
     **/
    char* active;
    
    /**
     * @brief Inactive Tray Icon Image
     **/
    char* inactive;
} SkinTrayIcon;

typedef struct
{
    char* backImg;
    ConfigColor keyColor;
} SkinKeyboard;

/** 
* 配置文件结构,方便处理,结构固定
*/
typedef struct FcitxSkin
{
    GenericConfig config;
    SkinInfo skinInfo;
    SkinFont skinFont;
    SkinMainBar skinMainBar;
    SkinInputBar skinInputBar;
    SkinTrayIcon skinTrayIcon;    
    SkinMenu skinMenu;
    SkinKeyboard skinKeyboard;
    
    char** skinType;
    
    SkinImage* imageTable;
} FcitxSkin;

int LoadSkinConfig(FcitxSkin* sc, char** skinType);
void DrawImage(cairo_t* c, cairo_surface_t* png, int x, int y, MouseE mouse);
void DrawInputBar(FcitxSkin* sc, struct InputWindow* inputWindow, struct Messages * msgup, struct Messages *msgdown ,unsigned int * iheight, unsigned int *iwidth);
SkinImage* LoadImage(FcitxSkin* sc, const char* name, boolean fallback);
void LoadInputMessage(FcitxSkin* sc, struct InputWindow* inputWindow, const char* font);
void InitSkinMenu(struct FcitxClassicUI* classicui);
void DisplaySkin(struct FcitxClassicUI* classicui, char * skinname);
void ParsePlacement(UT_array* sps, char* placment);
void DrawResizableBackground(cairo_t *c,
                             cairo_surface_t *background,
                             int height,
                             int width,
                             int marginLeft,
                             int marginTop,
                             int marginRight,
                             int marginBottom
                            );
#define fcitx_cairo_set_color(c, color) cairo_set_source_rgb((c), (color)->r, (color)->g, (color)->b)

CONFIG_BINDING_DECLARE(FcitxSkin);
#endif


