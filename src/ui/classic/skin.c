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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/**
 * @file   skin.c
 * @author Yuking yuking_net@sohu.com  t3swing  t3swing@sina.com
 *
 * @date   2009-10-9
 *
 * @brief  皮肤设置相关定义及初始化加载工作
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

static const UT_icd place_icd = {sizeof(SkinPlacement), NULL, NULL, NULL };

static boolean SkinMenuAction(FcitxUIMenu* menu, int index);
static void UpdateSkinMenuShell(FcitxUIMenu* menu);
static void UnloadImage(FcitxSkin* skin);

CONFIG_DESC_DEFINE(GetSkinDesc, "skin.desc")

/**
@加载皮肤配置文件
*/
int LoadSkinConfig(FcitxSkin* sc, char** skinType)
{
    FILE    *fp;
    char  buf[PATH_MAX] = {0};
    boolean    isreload = False;
    int ret = 0;
    if (sc->config.configFile) {
        utarray_done(&sc->skinMainBar.skinPlacement);
        FreeConfigFile(sc->config.configFile);
        free(sc->skinInfo.skinName);
        free(sc->skinInfo.skinVersion);
        free(sc->skinInfo.skinAuthor);
        free(sc->skinInfo.skinDesc);
        free(sc->skinMainBar.backImg);
        free(sc->skinMainBar.logo);
        free(sc->skinMainBar.eng);
        free(sc->skinMainBar.active);
        free(sc->skinMainBar.placement);
        free(sc->skinInputBar.backImg);
        free(sc->skinInputBar.backArrow);
        free(sc->skinInputBar.forwardArrow);
        free(sc->skinTrayIcon.active);
        free(sc->skinTrayIcon.inactive);
        free(sc->skinMenu.backImg);
        free(sc->skinKeyboard.backImg);
        UnloadImage(sc);
    }
    memset(sc, 0, sizeof(FcitxSkin));
    utarray_init(&sc->skinMainBar.skinPlacement, &place_icd);

reload:
    //获取配置文件的绝对路径
    {
        if (!isreload) {
            snprintf(buf, PATH_MAX, "%s/fcitx_skin.conf", *skinType);
            buf[PATH_MAX - 1] = '\0';

            fp = GetXDGFileWithPrefix("skin", buf, "r", NULL);
        } else {
            FcitxLog(INFO, PKGDATADIR "/skin/default/fcitx_skin.conf");
            fp = fopen(PKGDATADIR "/skin/default/fcitx_skin.conf", "r");
        }
    }

    if (fp) {
        ConfigFile *cfile;
        ConfigFileDesc* skinDesc = GetSkinDesc();
        if (sc->config.configFile == NULL) {
            cfile = ParseConfigFileFp(fp, skinDesc);
        } else {
            cfile = sc->config.configFile;
            cfile = ParseIniFp(fp, cfile);
        }
        if (!cfile) {
            fclose(fp);
            fp = NULL;
        } else {
            FcitxSkinConfigBind(sc, cfile, skinDesc);
            ConfigBindSync((GenericConfig*)sc);
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

SkinImage* LoadImage(FcitxSkin* sc, const char* name, boolean fallback)
{
    char buf[PATH_MAX];
    cairo_surface_t *png = NULL;
    SkinImage *image = NULL;

    HASH_FIND_STR(sc->imageTable, name, image);
    if (image != NULL)
        return image;
    if (strlen(name) > 0 && strcmp(name , "NONE") != 0) {
        char *skintype = strdup(*sc->skinType);
        size_t len;
        char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/skin" , DATADIR, PACKAGE "/skin");
        char *filename;
        while (True) {
            snprintf(buf, PATH_MAX, "%s/%s", skintype, name);
            buf[PATH_MAX - 1] = '\0';

            FILE* fp = GetXDGFile(buf, path, "r", len, &filename);

            Bool flagNoFile = (fp == NULL);
            if (fp) {
                fclose(fp);

                png = cairo_image_surface_create_from_png(filename);
                break;
            }
            if (flagNoFile && (!fallback || strcmp(skintype, "default") == 0)) {
                png = NULL;
                break;
            }

            free(filename);
            free(skintype);
            skintype = strdup("default");
        }
        free(filename);
        free(skintype);
        FreeXDGPath(path);
    }

    if (png != NULL) {
        image = fcitx_malloc0(sizeof(SkinImage));
        image->name = strdup(name);
        image->image = png;
        HASH_ADD_KEYPTR(hh, sc->imageTable, image->name, strlen(image->name), image);
        return image;
    }
    return NULL;
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
    if (png != NULL)
        cairo_surface_destroy(*png);
    *png = NULL;
}

/**
*输入条的绘制非常注重效率,画笔在绘图过程中不释放
*/
void LoadInputMessage(FcitxSkin* sc, InputWindow* inputWindow, const char* font)
{
    int i = 0;

    ConfigColor cursorColor = sc->skinInputBar.cursorColor;

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
        SetFontContext(inputWindow->c_font[i], font, sc->skinFont.fontSize);
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

void DrawInputBar(FcitxSkin* sc, InputWindow* inputWindow, int iCursorPos, Messages * msgup, Messages *msgdown , unsigned int * iheight, unsigned int *iwidth)
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
    int iChar = iCursorPos;
    int strWidth = 0, strHeight = 0;

    SkinImage *inputimg, *prev, *next;
    inputimg = LoadImage(sc, sc->skinInputBar.backImg, false);
    prev = LoadImage(sc, sc->skinInputBar.backArrow, false);
    next = LoadImage(sc, sc->skinInputBar.forwardArrow, false);

    if (!IsMessageChanged(msgup) && !IsMessageChanged(msgdown))
        return;

    inputWidth = 0;
#ifdef _ENABLE_PANGO /* special case which only macro unable to handle */
    SetFontContext(dummy, inputWindow->owner->font, sc->skinFont.fontSize);
#endif

    for (i = 0; i < GetMessageCount(msgup) ; i++) {
        char *trans = ProcessOutputFilter(instance, GetMessageString(msgup, i));
        if (trans)
            strUp[i] = trans;
        else
            strUp[i] = GetMessageString(msgup, i);
        posUpX[i] = sc->skinInputBar.marginLeft + inputWidth;

        StringSizeWithContext(inputWindow->c_font[GetMessageType(msgup, i)], strUp[i], &strWidth, &strHeight);

        posUpY[i] = sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos - strHeight;
        inputWidth += strWidth;
        if (FcitxInputStateGetShowCursor(input)) {
            int length = strlen(GetMessageString(msgup, i));
            if (iChar >= 0) {
                if (iChar < length) {
                    char strTemp[MESSAGE_MAX_LENGTH];
                    char *strGBKT = NULL;
                    strncpy(strTemp, strUp[i], iChar);
                    strTemp[iChar] = '\0';
                    strGBKT = strTemp;
                    StringSizeWithContext(inputWindow->c_font[GetMessageType(msgup, i)], strGBKT, &strWidth, &strHeight);
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
    for (i = 0; i < GetMessageCount(msgdown) ; i++) {
        char *trans = ProcessOutputFilter(instance, GetMessageString(msgdown, i));
        if (trans)
            strDown[i] = trans;
        else
            strDown[i] = GetMessageString(msgdown, i);

        if (inputWindow->owner->bVerticalList) { /* vertical */
            if (GetMessageType(msgdown, i) == MSG_INDEX) {
                if (currentX > outputWidth)
                    outputWidth = currentX;
                if (i != 0) {
                    outputHeight += sc->skinFont.fontSize + 2;
                    currentX = 0;
                }
            }
            posDownX[i] = sc->skinInputBar.marginLeft + currentX;
            StringSizeWithContext(inputWindow->c_font[GetMessageType(msgdown, i)], strDown[i], &strWidth, &strHeight);
            currentX += strWidth;
            posDownY[i] =  sc->skinInputBar.marginTop + sc->skinInputBar.iOutputPos + outputHeight - strHeight;
        } else { /* horizontal */
            posDownX[i] = sc->skinInputBar.marginLeft + outputWidth;
            StringSizeWithContext(inputWindow->c_font[GetMessageType(msgdown, i)], strDown[i], &strWidth, &strHeight);
            posDownY[i] = sc->skinInputBar.marginTop + sc->skinInputBar.iOutputPos - strHeight;
            outputWidth += strWidth;
        }
    }
    if (inputWindow->owner->bVerticalList && currentX > outputWidth)
        outputWidth = currentX;

    newHeight = sc->skinInputBar.marginTop + sc->skinInputBar.iOutputPos + outputHeight + sc->skinInputBar.marginBottom;

    newWidth = (inputWidth < outputWidth) ? outputWidth : inputWidth;
    newWidth += sc->skinInputBar.marginLeft + sc->skinInputBar.marginRight;

    /* round to ROUND_SIZE in order to decrease resize */
    newWidth = (newWidth / ROUND_SIZE) * ROUND_SIZE + ROUND_SIZE;

    //输入条长度应该比背景图长度要长,比最大长度要短
    newWidth = (newWidth >= INPUT_BAR_MAX_WIDTH) ? INPUT_BAR_MAX_WIDTH : newWidth;
    if (inputWindow->owner->bVerticalList) { /* vertical */
        newWidth = (newWidth < INPUT_BAR_VMIN_WIDTH) ? INPUT_BAR_VMIN_WIDTH : newWidth;
    } else {
        newWidth = (newWidth < INPUT_BAR_HMIN_WIDTH) ? INPUT_BAR_HMIN_WIDTH : newWidth;
    }

    *iwidth = newWidth;
    *iheight = newHeight;

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


    if (FcitxInputStateGetShowCursor(input)) {
        //画向前向后箭头
        if (prev && next) {
            cairo_set_source_surface(inputWindow->c_back, prev->image,
                                     newWidth - sc->skinInputBar.iBackArrowX ,
                                     sc->skinInputBar.iBackArrowY);
            if (CandidateWordHasPrev(FcitxInputStateGetCandidateList(input)))
                cairo_paint(inputWindow->c_back);
            else
                cairo_paint_with_alpha(inputWindow->c_back, 0.5);

            //画向前箭头
            cairo_set_source_surface(inputWindow->c_back, next->image,
                                     newWidth - sc->skinInputBar.iForwardArrowX ,
                                     sc->skinInputBar.iForwardArrowY);
            if (CandidateWordHasNext(FcitxInputStateGetCandidateList(input)))
                cairo_paint(inputWindow->c_back);
            else
                cairo_paint_with_alpha(inputWindow->c_back, 0.5);
        }
    }

    for (i = 0; i < GetMessageCount(msgup) ; i++) {
        OutputStringWithContext(inputWindow->c_font[GetMessageType(msgup, i)], strUp[i], posUpX[i], posUpY[i]);
        if (strUp[i] != GetMessageString(msgup, i))
            free(strUp[i]);
    }

    for (i = 0; i < GetMessageCount(msgdown) ; i++) {
        OutputStringWithContext(inputWindow->c_font[GetMessageType(msgdown, i)], strDown[i], posDownX[i], posDownY[i]);
        if (strDown[i] != GetMessageString(msgdown, i))
            free(strDown[i]);
    }

    ResetFontContext();

    //画光标
    if (FcitxInputStateGetShowCursor(input)) {
        cairo_move_to(inputWindow->c_cursor, cursor_pos, sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos);
        cairo_line_to(inputWindow->c_cursor, cursor_pos, sc->skinInputBar.marginTop + sc->skinInputBar.iInputPos - FontHeightWithContext(inputWindow->c_font[0]) - 4);
        cairo_stroke(inputWindow->c_cursor);
    }

    cairo_destroy(c);
    SetMessageChanged(msgup, false);
    SetMessageChanged(msgdown, false);
}

void DisplaySkin(FcitxClassicUI* classicui, char * skinname)
{
    char *pivot = classicui->skinType;
    classicui->skinType = strdup(skinname);
    if (pivot)
        free(pivot);

    if (LoadSkinConfig(&classicui->skin, &classicui->skinType))
        EndInstance(classicui->owner);

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

void UnloadImage(FcitxSkin* skin)
{
    SkinImage *images = skin->imageTable;
    while (images) {
        SkinImage* curimage = images;
        HASH_DEL(images, curimage);
        free(curimage->name);
        cairo_surface_destroy(curimage->image);
        free(curimage);
    }
    skin->imageTable = NULL;
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
    struct stat fileStat;
    size_t len;
    char pathBuf[PATH_MAX];
    char **skinPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", PACKAGE "/skin" , DATADIR, PACKAGE "/skin");
    for (i = 0; i < len; i++) {
        dir = opendir(skinPath[i]);
        if (dir == NULL)
            continue;

        while ((drt = readdir(dir)) != NULL) {
            if (strcmp(drt->d_name , ".") == 0 || strcmp(drt->d_name, "..") == 0)
                continue;
            sprintf(pathBuf, "%s/%s", skinPath[i], drt->d_name);

            if (stat(pathBuf, &fileStat) == -1) {
                continue;
            }
            if (fileStat.st_mode & S_IFDIR) {
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

    FreeXDGPath(skinPath);

    return;
}

void InitSkinMenu(FcitxClassicUI* classicui)
{
    utarray_init(&classicui->skinBuf, &ut_str_icd);
    strcpy(classicui->skinMenu.candStatusBind, "skin");
    strcpy(classicui->skinMenu.name, _("Skin"));
    utarray_init(&classicui->skinMenu.shell, &menuICD);

    classicui->skinMenu.UpdateMenuShell = UpdateSkinMenuShell;
    classicui->skinMenu.MenuAction = SkinMenuAction;
    classicui->skinMenu.priv = classicui;
    classicui->skinMenu.isSubMenu = false;
}

boolean SkinMenuAction(FcitxUIMenu* menu, int index)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    MenuShell* shell = (MenuShell*) utarray_eltptr(&menu->shell, index);
    if (shell)
        DisplaySkin(classicui, shell->tipstr);
    return true;
}

void UpdateSkinMenuShell(FcitxUIMenu* menu)
{
    FcitxClassicUI* classicui = (FcitxClassicUI*) menu->priv;
    LoadSkinDirectory(classicui);
    ClearMenuShell(menu);
    char **s;
    int i = 0;

    for (s = (char**) utarray_front(&classicui->skinBuf);
            s != NULL;
            s = (char**) utarray_next(&classicui->skinBuf, s)) {
        if (strcmp(*s, classicui->skinType) == 0) {
            menu->mark = i;
        }
        AddMenuShell(menu, *s, MENUTYPE_SIMPLE, NULL);
        i ++;
    }

}

void ParsePlacement(UT_array* sps, char* placment)
{
    UT_array* array = SplitString(placment, ';');
    char** str;
    utarray_clear(sps);
    for (str = (char**) utarray_front(array);
            str != NULL;
            str = (char**) utarray_next(array, str)) {
        char* s = *str;
        char* p = strchr(s, ':');
        if (p == NULL)
            continue;
        if ((strchr(s, ':') - s) > MAX_STATUS_NAME)
            continue;

        int len = p - s;
        SkinPlacement sp;
        strncpy(sp.name, s, len);
        sp.name[len] = '\0';
        int ret = sscanf(p + 1, "%d,%d", &sp.x, &sp.y);
        if (ret != 2)
            continue;
        utarray_push_back(sps, &sp);
    }

    utarray_free(array);
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
