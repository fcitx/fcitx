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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
/**
 * @file   skin.h
 * @author t3swing  t3swing@sina.com
 *
 * @date   2009-10-9
 *
 *  皮肤设置相关定义及初始化加载工作
 *
 *
 */

#ifndef _SKIN_H
#define _SKIN_H

#define SIZEX 800
#define SIZEY 200
#include "fcitx-utils/uthash.h"
#include <cairo.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx/ui.h"

struct _XlibMenu;
struct _InputWindow;
struct _Messages;
struct _FcitxClassicUI;

typedef enum _FillRule {
    F_COPY = 0,
    F_RESIZE = 1
} FillRule;

typedef enum _MouseE {
    RELEASE,//鼠标释放状态
    PRESS,//鼠标按下
    MOTION//鼠标停留
} MouseE;

typedef struct _SkinImage {
    char *name;
    cairo_surface_t *image;
    boolean textIcon;
    UT_hash_handle hh;
} SkinImage;

typedef struct _SkinInfo {
    char *skinName;
    char *skinVersion;
    char *skinAuthor;
    char *skinDesc;
} SkinInfo;

typedef struct _SkinFont {
    boolean respectDPI;
    int fontSize;
    int menuFontSize;
    FcitxConfigColor fontColor[7];
    FcitxConfigColor menuFontColor[2];
} SkinFont;

typedef struct _SkinMenu {
    char* backImg;
    int marginTop;
    int marginBottom;
    int marginLeft;
    int marginRight;
    FcitxConfigColor activeColor;
    FcitxConfigColor lineColor;
    FillRule fillV;
    FillRule fillH;
} SkinMenu;

/**
 * The Main Window Skin description
 **/
typedef struct _SkinMainBar {
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
    FillRule fillV;
    FillRule fillH;
    boolean bUseCustomTextIconColor;
    FcitxConfigColor textIconColor[2];
} SkinMainBar;

typedef struct _SkinInputBar {
    char* backImg;
    FcitxConfigColor cursorColor;
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
    FillRule fillV;
    FillRule fillH;
} SkinInputBar;

typedef struct _SkinPlacement {
    char *name;;
    int x;
    int y;
    UT_hash_handle hh;
} SkinPlacement;

/**
 * Tray Icon Image
 **/
typedef struct _SkinTrayIcon {
    /**
     * Active Tray Icon Image
     **/
    char* active;

    /**
     * Inactive Tray Icon Image
     **/
    char* inactive;
} SkinTrayIcon;

typedef struct _SkinKeyboard {
    char* backImg;
    FcitxConfigColor keyColor;
} SkinKeyboard;

/**
* 配置文件结构,方便处理,结构固定
*/
typedef struct _FcitxSkin {
    FcitxGenericConfig config;
    SkinInfo skinInfo;
    SkinFont skinFont;
    SkinMainBar skinMainBar;
    SkinInputBar skinInputBar;
    SkinTrayIcon skinTrayIcon;
    SkinMenu skinMenu;
    SkinKeyboard skinKeyboard;

    char** skinType;

    SkinImage* imageTable;
    SkinImage* trayImageTable;
} FcitxSkin;

FcitxConfigFileDesc* GetSkinDesc();
int LoadSkinConfig(FcitxSkin* sc, char** skinType);
void DrawImage(cairo_t* c, cairo_surface_t* png, int x, int y, MouseE mouse);
void DrawInputBar(FcitxSkin* sc, struct _InputWindow* inputWindow, boolean vertical, int iCursorPos, FcitxMessages* msgup, FcitxMessages* msgdown, unsigned int* iheight, unsigned int* iwidth);
SkinImage* LoadImage(FcitxSkin* sc, const char* name, int fallback);
SkinImage* LoadImageWithText(struct _FcitxClassicUI *classicui, FcitxSkin* sc, const char* name, const char* text, int w, int h, boolean active);
void LoadInputMessage(FcitxSkin* sc, struct _InputWindow* inputWindow, const char* font);
void InitSkinMenu(struct _FcitxClassicUI* classicui);
void DisplaySkin(struct _FcitxClassicUI* classicui, char * skinname);
void ParsePlacement(UT_array* sps, char* placment);
SkinImage* GetIMIcon(struct _FcitxClassicUI* classicui, FcitxSkin* sc, const char* fallbackIcon, int flag, boolean fallbackToDefault);
void DrawResizableBackground(cairo_t *c,
                             cairo_surface_t *background,
                             int height,
                             int width,
                             int marginLeft,
                             int marginTop,
                             int marginRight,
                             int marginBottom,
                             FillRule fillV,
                             FillRule fillH
                            );
#define fcitx_cairo_set_color(c, color) cairo_set_source_rgb((c), (color)->r, (color)->g, (color)->b)

CONFIG_BINDING_DECLARE(FcitxSkin);
#endif



// kate: indent-mode cstyle; space-indent on; indent-width 0;
