/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@gmail.com                                                     *
 *   Copyright (C) 2009~2010 by Yuking                                     *
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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

/**
 * @file   skin.c
 * @author Yuking yuking_net@sohu.com  t3swing  t3swing@sina.com
 *
 * @date   2009-10-9
 *
 *  Skin setting related code and draw code.
 *
 *
 */
#include <limits.h>
#include <cairo.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libintl.h>
#include <ctype.h>

#include "fcitx/fcitx.h"

#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/xdg.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utarray.h"

#include "classicui.h"
#include "skin.h"
#include "MenuWindow.h"
#include "InputWindow.h"
#include "MainWindow.h"
#include "TrayWindow.h"
#include "fcitx/ui.h"
#include "fcitx/frontend.h"
#include "fcitx-utils/utils.h"
#include "fcitx/instance.h"
#include "ui/cairostuff/font.h"
#include "fcitx/hook.h"
#include "fcitx/candidate.h"

static const UT_icd place_icd = { sizeof(SkinPlacement), NULL, NULL, NULL };

static boolean SkinMenuAction(FcitxUIMenu* menu, int index);
static void UpdateSkinMenu(FcitxUIMenu* menu);
static void UnloadImage(FcitxSkin* skin);
static void UnloadSingleImage(FcitxSkin* sc, const char* name);

CONFIG_DESC_DEFINE(GetSkinDesc, "skin.desc")

/**
@加载皮肤配置文件
*/
int LoadSkinConfig(FcitxSkin* sc, char** skinType)
{
    FILE    *fp;
    boolean    isreload = False;
    int ret = 0;
    if (sc->config.configFile) {
        utarray_done(&sc->skinMainBar.skinPlacement);
        FcitxConfigFree(&sc->config);
        UnloadImage(sc);
    }
    memset(sc, 0, sizeof(FcitxSkin));
    utarray_init(&sc->skinMainBar.skinPlacement, &place_icd);

reload:
    if (!isreload) {
        char *buf;
        fcitx_utils_alloc_cat_str(buf, *skinType, "/fcitx_skin.conf");
        fp = FcitxXDGGetFileWithPrefix("skin", buf, "r", NULL);
        free(buf);
    } else {
        char *path = fcitx_utils_get_fcitx_path_with_filename(
            "pkgdatadir", "/skin/default/fcitx_skin.conf");
        fp = fopen(path, "r");
        free(path);
    }

    if (fp) {
        FcitxConfigFile *cfile;
        FcitxConfigFileDesc* skinDesc = GetSkinDesc();
        if (sc->config.configFile == NULL) {
            cfile = FcitxConfigParseConfigFileFp(fp, skinDesc);
        } else {
            cfile = sc->config.configFile;
            cfile = FcitxConfigParseIniFp(fp, cfile);
        }
        if (!cfile) {
            fclose(fp);
            fp = NULL;
        } else {
            FcitxSkinConfigBind(sc, cfile, skinDesc);
            FcitxConfigBindSync((FcitxGenericConfig*)sc);
        }
    }

    if (!fp) {
        if (isreload) {
            FcitxLog(FATAL, _("Can not load default skin, is installion correct?"));
            perror("fopen");
            ret = 1;    // 如果安装目录里面也没有配置文件，那就只好告诉用户，无法运行了
        } else {
            perror("fopen");
            FcitxLog(WARNING, _("Can not load skin %s, return to default"), *skinType);
            if (*skinType)
                free(*skinType);
            *skinType = strdup("default");
            isreload = true;
            goto reload;
        }
    }

    if (fp)
        fclose(fp);
    sc->skinType = skinType;

    return ret;

}

SkinImage* LoadImageWithText(FcitxClassicUI* classicui, FcitxSkin* sc, const char* name, const char* text, int w, int h, boolean active)
{
    if (!text || *text == '\0')
        return NULL;

    UnloadSingleImage(sc, name);

    int len = fcitx_utf8_char_len(text);
    if (len == 1 && text[len] && fcitx_utf8_char_len(text + len) == 1)
        len = 2;

    char* iconText = strndup(text, len);

    FcitxLog(DEBUG, "%s", iconText);

    cairo_surface_t* newsurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* c = cairo_create(newsurface);

    int min = w > h? h: w;
    min = min * 0.7;

    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(c ,1, 1, 1, 0.0);
    cairo_paint(c);

    FcitxConfigColor color;
    if (sc->skinMainBar.bUseCustomTextIconColor) {
        if (active)
            color = sc->skinMainBar.textIconColor[0];
        else
            color = sc->skinMainBar.textIconColor[1];
    }
    else
        color = sc->skinFont.menuFontColor[1];

    int textw, texth;
    StringSizeStrict(iconText, classicui->font, min, false, &textw, &texth);

    OutputString(c, iconText, classicui->font, min, false, (w - textw) * 0.5, 0, &color);

    cairo_destroy(c);
    SkinImage* image = fcitx_utils_malloc0(sizeof(SkinImage));
    image->name = strdup(name);
    image->image = newsurface;
    image->textIcon = true;
    HASH_ADD_KEYPTR(hh, sc->imageTable, image->name, strlen(image->name), image);
    return image;
}

SkinImage* LoadImageFromTable(SkinImage** imageTable, const char* skinType, const char* name, int flag)
{
    cairo_surface_t *png = NULL;
    SkinImage *image = NULL;
    char *buf;
    fcitx_utils_alloc_cat_str(buf, "skin/", skinType);
    const char* fallbackChainNoFallback[] = { buf };
    const char* fallbackChainPanel[] = { buf, "skin/default" };
    const char* fallbackChainTray[] = { "imicon" };
    const char* fallbackChainPanelIMIcon[] = { buf, "imicon", "skin/default" };

    HASH_FIND_STR(*imageTable, name, image);
    if (image != NULL) {
        free(buf);
        return image;
    }

    const char** fallbackChain;
    int fallbackSize;
    switch(flag) {
        case 1:
            fallbackChain = fallbackChainPanel;
            fallbackSize = 2;
            break;
        case 2:
            fallbackChain = fallbackChainTray;
            fallbackSize = 1;
            break;
        case 3:
            fallbackChain = fallbackChainPanelIMIcon;
            fallbackSize = 3;
            break;
        case 0:
        default:
            /* fall through */
            fallbackChain = fallbackChainNoFallback;
            fallbackSize = 1;
            break;
    }

    if (strlen(name) > 0 && strcmp(name , "NONE") != 0) {
        int i = 0;
        for (i = 0; i < fallbackSize; i ++) {
            char* filename;
            const char* skintype = fallbackChain[i];

            FILE* fp = FcitxXDGGetFileWithPrefix(skintype, name, "r", &filename);
            if (fp) {
                png = cairo_image_surface_create_from_png(filename);
                if (cairo_surface_status (png)) {
                    png = NULL;
                }
            }

            free(filename);
            if (png)
                break;
        }
    }
    free(buf);

    if (png != NULL) {
        image = fcitx_utils_new(SkinImage);
        image->name = strdup(name);
        image->image = png;
        HASH_ADD_KEYPTR(hh, *imageTable, image->name,
                        strlen(image->name), image);
        return image;
    }
    return NULL;
}

SkinImage* LoadImage(FcitxSkin* sc, const char* name, int flag)
{
    if (flag == 2)
        return LoadImageFromTable(&sc->trayImageTable, *sc->skinType, name, flag);
    else
        return LoadImageFromTable(&sc->imageTable, *sc->skinType, name, flag);
}

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
                            )
{
    int resizeHeight = cairo_image_surface_get_height(background) - marginTop - marginBottom;
    int resizeWidth = cairo_image_surface_get_width(background) - marginLeft - marginRight;

    if (resizeHeight <= 0)
        resizeHeight = 1;

    if (resizeWidth <= 0)
        resizeWidth = 1;
    cairo_save(c);

    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(c, background, 0, 0);


    /* 九宫格
     * 7 8 9
     * 4 5 6
     * 1 2 3
     */

    /* part 1 */
    cairo_save(c);
    cairo_translate(c, 0, height - marginBottom);
    cairo_set_source_surface(c, background, 0, -marginTop - resizeHeight);
    cairo_rectangle(c, 0, 0, marginLeft, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 3 */
    cairo_save(c);
    cairo_translate(c, width - marginRight, height - marginBottom);
    cairo_set_source_surface(c, background, -marginLeft - resizeWidth, -marginTop - resizeHeight);
    cairo_rectangle(c, 0, 0, marginRight, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);


    /* part 7 */
    cairo_save(c);
    cairo_rectangle(c, 0, 0, marginLeft, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 9 */
    cairo_save(c);
    cairo_translate(c, width - marginRight, 0);
    cairo_set_source_surface(c, background, -marginLeft - resizeWidth, 0);
    cairo_rectangle(c, 0, 0, marginRight, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    /* part 2 & 8 */
    {
        if (fillH == F_COPY) {
            int repaint_times = (width - marginLeft - marginRight) / resizeWidth;
            int remain_width = (width - marginLeft - marginRight) % resizeWidth;
            int i = 0;

            for (i = 0; i < repaint_times; i++) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth, 0);
                cairo_set_source_surface(c, background, -marginLeft, 0);
                cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  marginLeft + i * resizeWidth, height - marginBottom);
                cairo_set_source_surface(c, background,  -marginLeft, -marginTop - resizeHeight);
                cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_width != 0) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + repaint_times * resizeWidth, 0);
                cairo_set_source_surface(c, background, -marginLeft, 0);
                cairo_rectangle(c, 0, 0, remain_width, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  marginLeft + repaint_times * resizeWidth, height - marginBottom);
                cairo_set_source_surface(c, background,  -marginLeft, -marginTop - resizeHeight);
                cairo_rectangle(c, 0, 0, remain_width, marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        } else {
            cairo_save(c);
            cairo_translate(c, marginLeft, 0);
            cairo_scale(c, (double)(width - marginLeft - marginRight) / (double)resizeWidth, 1);
            cairo_set_source_surface(c, background, -marginLeft, 0);
            cairo_rectangle(c, 0, 0, resizeWidth, marginTop);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);

            cairo_save(c);
            cairo_translate(c, marginLeft, height - marginBottom);
            cairo_scale(c, (double)(width - marginLeft - marginRight) / (double)resizeWidth, 1);
            cairo_set_source_surface(c, background, -marginLeft, -marginTop - resizeHeight);
            cairo_rectangle(c, 0, 0, resizeWidth, marginBottom);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
        }
    }

    /* part 4 & 6 */
    {
        if (fillV == F_COPY) {
            int repaint_times = (height - marginTop - marginBottom) / resizeHeight;
            int remain_height = (height - marginTop - marginBottom) % resizeHeight;
            int i = 0;

            for (i = 0; i < repaint_times; i++) {
                /* part 4 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + i * resizeHeight);
                cairo_set_source_surface(c, background, 0, -marginTop);
                cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 6 */
                cairo_save(c);
                cairo_translate(c, width - marginRight,  marginTop + i * resizeHeight);
                cairo_set_source_surface(c, background, -marginLeft - resizeWidth,  -marginTop);
                cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_height != 0) {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + repaint_times * resizeHeight);
                cairo_set_source_surface(c, background, 0, -marginTop);
                cairo_rectangle(c, 0, 0, marginLeft, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);

                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  width - marginRight,  marginTop + repaint_times * resizeHeight);
                cairo_set_source_surface(c, background, -marginLeft - resizeWidth,  -marginTop);
                cairo_rectangle(c, 0, 0, marginRight, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        } else {
            cairo_save(c);
            cairo_translate(c, 0, marginTop);
            cairo_scale(c, 1, (double)(height - marginTop - marginBottom) / (double)resizeHeight);
            cairo_set_source_surface(c, background, 0, -marginTop);
            cairo_rectangle(c, 0, 0, marginLeft, resizeHeight);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);

            cairo_save(c);
            cairo_translate(c, width - marginRight, marginTop);
            cairo_scale(c, 1, (double)(height - marginTop - marginBottom) / (double)resizeHeight);
            cairo_set_source_surface(c, background, -marginLeft - resizeWidth, -marginTop);
            cairo_rectangle(c, 0, 0, marginRight, resizeHeight);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
        }
    }

    /* part 5 */
    {
        int repaintH = 0, repaintV = 0;
        int remainW = 0, remainH = 0;
        double scaleX = 1.0, scaleY = 1.0;

        if (fillH == F_COPY) {
            repaintH = (width - marginLeft - marginRight) / resizeWidth + 1;
            remainW = (width - marginLeft - marginRight) % resizeWidth;
        } else {
            repaintH = 1;
            scaleX = (double)(width - marginLeft - marginRight) / (double)resizeWidth;
        }

        if (fillV == F_COPY) {
            repaintV = (height - marginTop - marginBottom) / (double)resizeHeight + 1;
            remainH = (height - marginTop - marginBottom) % resizeHeight;
        } else {
            repaintV = 1;
            scaleY = (double)(height - marginTop - marginBottom) / (double)resizeHeight;
        }


        int i, j;
        for (i = 0; i < repaintH; i ++) {
            for (j = 0; j < repaintV; j ++) {
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth , marginTop + j * resizeHeight);
                cairo_scale(c, scaleX, scaleY);
                cairo_set_source_surface(c, background, -marginLeft, -marginTop);
                int w = resizeWidth, h = resizeHeight;

                if (fillV == F_COPY && j == repaintV - 1)
                    h = remainH;

                if (fillH == F_COPY && i == repaintH - 1)
                    w = remainW;

                cairo_rectangle(c, 0, 0, w, h);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
    }
    cairo_restore(c);
}

void DestroyImage(cairo_surface_t ** png)
{
    if (png != NULL) {
        cairo_surface_destroy(*png);
        *png = NULL;
    }
}

/**
*输入条的绘制非常注重效率,画笔在绘图过程中不释放
*/
void LoadInputMessage(FcitxSkin* sc, InputWindow* inputWindow, const char* font)
{
    int i = 0;

    FcitxConfigColor cursorColor = sc->skinInputBar.cursorColor;

    if (inputWindow->c_back) {
        cairo_destroy(inputWindow->c_back);
        inputWindow->c_back = NULL;
    }

    for (i = 0; i < 7 ; i ++) {
        if (inputWindow->c_font[i]) {
            cairo_destroy(inputWindow->c_font[i]);
            inputWindow->c_font[i] = NULL;
        }
    }
    inputWindow->c_font[7] = NULL;
    if (inputWindow->c_cursor) {
        cairo_destroy(inputWindow->c_cursor);
        inputWindow->c_cursor = NULL;
    }

    //输入条背景图画笔
    inputWindow->c_back = cairo_create(inputWindow->cs_input_bar);

    for (i = 0; i < 7 ; i ++) {
        inputWindow->c_font[i] = cairo_create(inputWindow->cs_input_bar);
        fcitx_cairo_set_color(inputWindow->c_font[i], &sc->skinFont.fontColor[i]);
#ifndef _ENABLE_PANGO
        SetFontContext(inputWindow->c_font[i],
                       font,
                       inputWindow->owner->fontSize > 0 ? inputWindow->owner->fontSize : sc->skinFont.fontSize,
                       dpi);
#endif
    }
    inputWindow->c_font[7] = inputWindow->c_font[0];

    //光标画笔
    inputWindow->c_cursor = cairo_create(inputWindow->cs_input_bar);
    fcitx_cairo_set_color(inputWindow->c_cursor, &cursorColor);
    cairo_set_line_width(inputWindow->c_cursor, 1);
}

void DrawImage(cairo_t *c, cairo_surface_t * png, int x, int y, MouseE mouse)
{
    if (!png)
        return;

    cairo_save(c);

    if (mouse == MOTION) {
        cairo_set_source_surface(c, png, x, y);
        cairo_paint_with_alpha(c, 0.7);

    } else if (mouse == PRESS) {
        cairo_set_operator(c, CAIRO_OPERATOR_OVER);
        cairo_translate(c, x + (int)(cairo_image_surface_get_width(png) * 0.2 / 2), y + (int)(cairo_image_surface_get_height(png) * 0.2 / 2));
        cairo_scale(c, 0.8, 0.8);
        cairo_set_source_surface(c, png, 0, 0);
        cairo_paint(c);
    } else { //if( img.mouse == RELEASE)
        cairo_set_source_surface(c, png, x, y);
        cairo_paint(c);
    }

    cairo_restore(c);
}

void DrawInputBar(FcitxSkin* sc, InputWindow* inputWindow, boolean vertical, int iCursorPos, FcitxMessages * msgup, FcitxMessages *msgdown , unsigned int * iheight, unsigned int *iwidth)
{
    int i;
    char *strUp[MAX_MESSAGE_COUNT];
    char *strDown[MAX_MESSAGE_COUNT];
    int posUpX[MAX_MESSAGE_COUNT], posUpY[MAX_MESSAGE_COUNT];
    int posDownX[MAX_MESSAGE_COUNT], posDownY[MAX_MESSAGE_COUNT];
    int oldHeight = *iheight, oldWidth = *iwidth;
    int newHeight = 0, newWidth = 0;
    int cursor_pos = 0;
    int inputWidth = 0, outputWidth = 0;
    int outputHeight = 0;
    cairo_t *c = NULL;
    FcitxInputState* input = FcitxInstanceGetInputState(inputWindow->owner->owner);
    FcitxInstance* instance = inputWindow->owner->owner;
    FcitxClassicUI* classicui = inputWindow->owner;
    int iChar = iCursorPos;
    int strWidth = 0, strHeight = 0;

    SkinImage *inputimg, *prev, *next;
    inputimg = LoadImage(sc, sc->skinInputBar.backImg, false);
    prev = LoadImage(sc, sc->skinInputBar.backArrow, false);
    next = LoadImage(sc, sc->skinInputBar.forwardArrow, false);

    if (!FcitxMessagesIsMessageChanged(msgup) && !FcitxMessagesIsMessageChanged(msgdown))
        return;

    inputWidth = 0;
    int dpi = sc->skinFont.respectDPI? classicui->dpi : 0;
    FCITX_UNUSED(dpi);
#ifdef _ENABLE_PANGO /* special case which only macro unable to handle */
    SetFontContext(dummy, inputWindow->owner->font, inputWindow->owner->fontSize > 0 ? inputWindow->owner->fontSize : sc->skinFont.fontSize, dpi);
#endif

    int fontHeight = FontHeightWithContext(inputWindow->c_font[0], dpi);
    for (i = 0; i < FcitxMessagesGetMessageCount(msgup) ; i++) {
        char *trans = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(msgup, i));
        if (trans)
            strUp[i] = trans;
        else
            strUp[i] = FcitxMessagesGetMessageString(msgup, i);
        posUpX[i] = sc->skinInputBar.marginLeft + inputWidth;

        StringSizeWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgup, i)], dpi, strUp[i], &strWidth, &strHeight);

        if (sc->skinFont.respectDPI)
            posUpY[i] = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos;
        else
            posUpY[i] = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos - strHeight;
        inputWidth += strWidth;
        if (FcitxInputStateGetShowCursor(input)) {
            int length = strlen(FcitxMessagesGetMessageString(msgup, i));
            if (iChar >= 0) {
                if (iChar < length) {
                    char strTemp[MESSAGE_MAX_LENGTH];
                    char *strGBKT = NULL;
                    strncpy(strTemp, strUp[i], iChar);
                    strTemp[iChar] = '\0';
                    strGBKT = strTemp;
                    StringSizeWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgup, i)], dpi, strGBKT, &strWidth, &strHeight);
                    cursor_pos = posUpX[i]
                                 + strWidth + 2;
                }
                iChar -= length;
            }
        }

    }

    if (iChar >= 0)
        cursor_pos = inputWidth + sc->skinInputBar.marginLeft;

    outputWidth = 0;
    outputHeight = 0;
    int currentX = 0;
    int offsetY;
    if (sc->skinFont.respectDPI)
        offsetY = sc->skinInputBar.marginTop
                 + (FcitxMessagesGetMessageCount(msgup) ? (sc->skinInputBar.iInputPos + fontHeight) : 0)
                 + (FcitxMessagesGetMessageCount(msgdown) ? sc->skinInputBar.iOutputPos : 0);
    else
        offsetY = sc->skinInputBar.marginTop + sc->skinInputBar.iOutputPos - fontHeight;
    for (i = 0; i < FcitxMessagesGetMessageCount(msgdown) ; i++) {
        char *trans = FcitxInstanceProcessOutputFilter(instance, FcitxMessagesGetMessageString(msgdown, i));
        if (trans)
            strDown[i] = trans;
        else
            strDown[i] = FcitxMessagesGetMessageString(msgdown, i);

        if (vertical) { /* vertical */
            if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX) {
                if (currentX > outputWidth)
                    outputWidth = currentX;
                if (i != 0) {
                    currentX = 0;
                }
            }
            posDownX[i] = sc->skinInputBar.marginLeft + currentX;
            StringSizeWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgdown, i)], dpi, strDown[i], &strWidth, &strHeight);
            if (FcitxMessagesGetMessageType(msgdown, i) == MSG_INDEX && i != 0)
                outputHeight += fontHeight + 2;
            currentX += strWidth;
        } else { /* horizontal */
            posDownX[i] = sc->skinInputBar.marginLeft + outputWidth;
            StringSizeWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgdown, i)], dpi, strDown[i], &strWidth, &strHeight);
            outputWidth += strWidth;
        }
        posDownY[i] = offsetY + outputHeight;
    }
    if (vertical && currentX > outputWidth)
        outputWidth = currentX;

    newHeight = offsetY + outputHeight + sc->skinInputBar.marginBottom + (FcitxMessagesGetMessageCount(msgdown) || !sc->skinFont.respectDPI ? fontHeight : 0);

    newWidth = (inputWidth < outputWidth) ? outputWidth : inputWidth;
    newWidth += sc->skinInputBar.marginLeft + sc->skinInputBar.marginRight;

    /* round to ROUND_SIZE in order to decrease resize */
    newWidth = (newWidth / ROUND_SIZE) * ROUND_SIZE + ROUND_SIZE;

    if (vertical) { /* vertical */
        newWidth = (newWidth < INPUT_BAR_VMIN_WIDTH) ? INPUT_BAR_VMIN_WIDTH : newWidth;
    } else {
        newWidth = (newWidth < INPUT_BAR_HMIN_WIDTH) ? INPUT_BAR_HMIN_WIDTH : newWidth;
    }

    *iwidth = newWidth;
    *iheight = newHeight;

    EnlargeCairoSurface(&inputWindow->cs_input_back, newWidth, newHeight);
    if (EnlargeCairoSurface(&inputWindow->cs_input_bar, newWidth, newHeight)) {
        LoadInputMessage(&classicui->skin, classicui->inputWindow, classicui->font);
    }

    if (oldHeight != newHeight || oldWidth != newWidth) {
        c = cairo_create(inputWindow->cs_input_back);
        DrawResizableBackground(c, inputimg->image, newHeight, newWidth,
                                sc->skinInputBar.marginLeft,
                                sc->skinInputBar.marginTop,
                                sc->skinInputBar.marginRight,
                                sc->skinInputBar.marginBottom,
                                sc->skinInputBar.fillV,
                                sc->skinInputBar.fillH
                               );
        cairo_destroy(c);
    }

    c = cairo_create(inputWindow->cs_input_bar);
    cairo_set_source_surface(c, inputWindow->cs_input_back, 0, 0);
    cairo_save(c);
    cairo_rectangle(c, 0, 0, newWidth, newHeight);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    cairo_set_operator(c, CAIRO_OPERATOR_OVER);

    FcitxCandidateWordList* candList = FcitxInputStateGetCandidateList(input);
    if (FcitxCandidateWordHasPrev(candList)
        || FcitxCandidateWordHasNext(candList)
    ) {
        //画向前向后箭头
        if (prev && next) {
            cairo_set_source_surface(inputWindow->c_back, prev->image,
                                     newWidth - sc->skinInputBar.iBackArrowX ,
                                     sc->skinInputBar.iBackArrowY);
            if (FcitxCandidateWordHasPrev(candList))
                cairo_paint(inputWindow->c_back);
            else
                cairo_paint_with_alpha(inputWindow->c_back, 0.5);

            //画向前箭头
            cairo_set_source_surface(inputWindow->c_back, next->image,
                                     newWidth - sc->skinInputBar.iForwardArrowX ,
                                     sc->skinInputBar.iForwardArrowY);
            if (FcitxCandidateWordHasNext(candList))
                cairo_paint(inputWindow->c_back);
            else
                cairo_paint_with_alpha(inputWindow->c_back, 0.5);
        }
    }

    for (i = 0; i < FcitxMessagesGetMessageCount(msgup) ; i++) {
        OutputStringWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgup, i)], dpi, strUp[i], posUpX[i], posUpY[i]);
        if (strUp[i] != FcitxMessagesGetMessageString(msgup, i))
            free(strUp[i]);
    }

    for (i = 0; i < FcitxMessagesGetMessageCount(msgdown) ; i++) {
        OutputStringWithContext(inputWindow->c_font[FcitxMessagesGetMessageType(msgdown, i)], dpi, strDown[i], posDownX[i], posDownY[i]);
        if (strDown[i] != FcitxMessagesGetMessageString(msgdown, i))
            free(strDown[i]);
    }

    int cursorY1, cursorY2;
    if (sc->skinFont.respectDPI) {
        cursorY1 = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos;
        cursorY2 = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos + fontHeight;
    }
    else {
        cursorY1 = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos - fontHeight;
        cursorY2 = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos;
    }

    //画光标
    if (FcitxMessagesGetMessageCount(msgup) && FcitxInputStateGetShowCursor(input)) {
        cairo_move_to(inputWindow->c_cursor, cursor_pos, cursorY1);
        cairo_line_to(inputWindow->c_cursor, cursor_pos, cursorY2);
        cairo_stroke(inputWindow->c_cursor);
    }

    ResetFontContext();

    cairo_destroy(c);
    FcitxMessagesSetMessageChanged(msgup, false);
    FcitxMessagesSetMessageChanged(msgdown, false);
}

void DisplaySkin(FcitxClassicUI* classicui, char * skinname)
{
    char *pivot = classicui->skinType;
    classicui->skinType = strdup(skinname);
    if (pivot)
        free(pivot);

    if (LoadSkinConfig(&classicui->skin, &classicui->skinType))
        FcitxInstanceEnd(classicui->owner);

#ifndef _ENABLE_PANGO
    GetValidFont(classicui->strUserLocale, &classicui->font);
    GetValidFont(classicui->strUserLocale, &classicui->menuFont);
#endif

    LoadInputMessage(&classicui->skin, classicui->inputWindow, classicui->font);

    DrawMainWindow(classicui->mainWindow);
    DrawInputWindow(classicui->inputWindow);
    DrawTrayWindow(classicui->trayWindow);

    SaveClassicUIConfig(classicui);
}

void FreeImageTable(SkinImage* table)
{
    SkinImage *images = table;
    while (images) {
        SkinImage* curimage = images;
        HASH_DEL(images, curimage);
        free(curimage->name);
        cairo_surface_destroy(curimage->image);
        free(curimage);
    }
}

void UnloadImage(FcitxSkin* skin)
{
    FreeImageTable(skin->imageTable);
    skin->imageTable = NULL;

    FreeImageTable(skin->trayImageTable);
    skin->trayImageTable = NULL;
}

void UnloadSingleImage(FcitxSkin* sc, const char* name)
{
    SkinImage *image;
    HASH_FIND_STR(sc->imageTable, name, image);
    if (image != NULL) {
        SkinImage* curimage = image;
        HASH_DEL(sc->imageTable, image);
        free(curimage->name);
        cairo_surface_destroy(curimage->image);
        free(curimage);
    }
}

//图片文件加载函数完成
/*-------------------------------------------------------------------------------------------------------------*/
//skin目录下读入skin的文件夹名

void LoadSkinDirectory(FcitxClassicUI* classicui)
{
    UT_array* skinBuf = &classicui->skinBuf;
    utarray_clear(skinBuf);
    int i ;
    DIR *dir;
    struct dirent *drt;
    size_t len;
    char **skinPath = FcitxXDGGetPathWithPrefix(&len, "skin");
    for (i = 0; i < len; i++) {
        dir = opendir(skinPath[i]);
        if (dir == NULL)
            continue;

        while ((drt = readdir(dir)) != NULL) {
            if (strcmp(drt->d_name , ".") == 0 ||
                strcmp(drt->d_name, "..") == 0)
                continue;
            char *pathBuf;
            fcitx_utils_alloc_cat_str(pathBuf, skinPath[i], "/", drt->d_name, "/fcitx_skin.conf");
            boolean result = fcitx_utils_isreg(pathBuf);
            free(pathBuf);
            if (result) {
                /* check duplicate name */
                int j = 0;
                for (; j < skinBuf->i; j++) {
                    char **name = (char**) utarray_eltptr(skinBuf, j);
                    if (strcmp(*name, drt->d_name) == 0)
                        break;
                }
                if (j == skinBuf->i) {
                    char *temp = drt->d_name;
                    utarray_push_back(skinBuf, &temp);
                }
            }
        }

        closedir(dir);
    }

    FcitxXDGFreePath(skinPath);

    return;
}

void InitSkinMenu(FcitxClassicUI* classicui)
{
    utarray_init(&classicui->skinBuf, fcitx_str_icd);
    FcitxMenuInit(&classicui->skinMenu);
    classicui->skinMenu.candStatusBind = NULL;
    classicui->skinMenu.name =  strdup(_("Skin"));

    classicui->skinMenu.UpdateMenu = UpdateSkinMenu;
    classicui->skinMenu.MenuAction = SkinMenuAction;
    classicui->skinMenu.priv = classicui;
    classicui->skinMenu.isSubMenu = false;
}

boolean SkinMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    FcitxMenuItem* shell = (FcitxMenuItem*) utarray_eltptr(&menu->shell, index);
    if (shell)
        DisplaySkin(classicui, shell->tipstr);
    return true;
}

void UpdateSkinMenu(FcitxUIMenu* menu)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    LoadSkinDirectory(classicui);
    FcitxMenuClear(menu);
    char **s;
    int i = 0;

    for (s = (char**) utarray_front(&classicui->skinBuf);
            s != NULL;
            s = (char**) utarray_next(&classicui->skinBuf, s)) {
        if (strcmp(*s, classicui->skinType) == 0) {
            menu->mark = i;
        }
        FcitxMenuAddMenuItem(menu, *s, MENUTYPE_SIMPLE, NULL);
        i ++;
    }

}

void ParsePlacement(UT_array* sps, char* placment)
{
    UT_array* array = fcitx_utils_split_string(placment, ';');
    char** str;
    utarray_clear(sps);
    for (str = (char**) utarray_front(array);
            str != NULL;
            str = (char**) utarray_next(array, str)) {
        char* s = *str;
        char* p = strchr(s, ':');
        if (p == NULL)
            continue;

        int len = p - s;
        SkinPlacement sp;
        sp.name = strndup(s, len);
        int ret = sscanf(p + 1, "%d,%d", &sp.x, &sp.y);
        if (ret != 2)
            continue;
        utarray_push_back(sps, &sp);
    }

    utarray_free(array);
}

SkinImage* GetIMIcon(FcitxClassicUI* classicui, FcitxSkin *sc, const char* fallbackIcon, int flag, boolean fallbackToDefault)
{
    FcitxIM* im = FcitxInstanceGetCurrentIM(classicui->owner);
    if (!im)
        return NULL;
    const char *path;
    char *tmpstr = NULL;
    if (im->strIconName[0] == '/') {
        path = im->strIconName;
    } else {
        fcitx_utils_alloc_cat_str(tmpstr, im->strIconName, ".png");
        path = tmpstr;
    }
    SkinImage *imicon = NULL;
    if (strncmp(im->uniqueName, "fcitx-keyboard-",
                strlen("fcitx-keyboard-")) == 0) {
        SkinImage* activeIcon = LoadImage(sc, fallbackIcon, fallbackToDefault);
        char temp[LANGCODE_LENGTH + 1] = { '\0', };
        char* iconText = 0;
        if (*im->langCode) {
            strncpy(temp, im->langCode, LANGCODE_LENGTH);
            iconText = temp;
            iconText[0] = toupper(iconText[0]);
        } else {
            iconText = im->uniqueName + strlen("fcitx-keyboard-");
        }
        imicon = LoadImageWithText(
            classicui, sc, path, iconText,
            cairo_image_surface_get_width(activeIcon->image),
            cairo_image_surface_get_height(activeIcon->image), true);
    }

    if (imicon == NULL)
        imicon = LoadImage(sc, path, flag);
    fcitx_utils_free(tmpstr);
    if (imicon == NULL) {
        imicon = LoadImage(sc, fallbackIcon, fallbackToDefault);
    } else {
        SkinImage* activeIcon = LoadImage(sc, fallbackIcon, fallbackToDefault);
        if (activeIcon) {
            ResizeSurface(&imicon->image,
                          cairo_image_surface_get_width(activeIcon->image),
                          cairo_image_surface_get_height(activeIcon->image));
        }
    }
    return imicon;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
