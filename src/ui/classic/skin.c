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
int LoadSkinConfig(FcitxSkin* sc, char** skinType, boolean fallback)
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
        if (!fallback) {
            return 1;
        }

        if (isreload) {
            FcitxLog(FATAL, _("Cannot load default skin, is installation correct?"));
            perror("fopen");
            ret = 1;    // 如果安装目录里面也没有配置文件，那就只好告诉用户，无法运行了
        } else {
            perror("fopen");
            FcitxLog(WARNING, _("Cannot load skin %s, return to default"), *skinType);
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

    if (name[0] == '@') {
        name++;
    }

    UnloadSingleImage(sc, name);

    int len = fcitx_utf8_char_len(text);
    if (len == 1 && text[len] && fcitx_utf8_char_len(text + len) == 1)
        len = 2;

    char* iconText = strndup(text, len);

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
    } else {
        color = sc->skinFont.menuFontColor[1];
    }

    int textw, texth;
    FcitxCairoTextContext* ctc = FcitxCairoTextContextCreate(c);
    FcitxCairoTextContextSet(ctc, classicui->font, min, false);
    FcitxCairoTextContextStringSizeStrict(ctc, iconText, &textw, &texth);

    FcitxCairoTextContextOutputString(ctc, iconText, (w - textw) * 0.5, 0, &color);

    free(iconText);
    FcitxCairoTextContextFree(ctc);

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

    if (name[0] == '@') {
        name ++;
    }

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
            char* filename = NULL;
            const char* skintype = fallbackChain[i];

            FILE* fp = FcitxXDGGetFileWithPrefix(skintype, name, "r", &filename);
            do {
                if (!fp) {
                    break;
                }
                fclose(fp);

                png = cairo_image_surface_create_from_png(filename);
                if (!png) {
                    break;
                }

                if (cairo_surface_status (png)) {
                    cairo_surface_destroy(png);
                    png = NULL;
                }
            } while(0);

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
                             int width,
                             int height,
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

void DisplaySkin(FcitxClassicUI* classicui, char * skinname)
{
    char *pivot = classicui->skinType;
    classicui->skinType = strdup(skinname);
    if (pivot)
        free(pivot);

    if (LoadSkinConfig(&classicui->skin, &classicui->skinType, /*fallback=*/true))
        FcitxInstanceEnd(classicui->owner);

#ifndef _ENABLE_PANGO
    GetValidFont(classicui->strUserLocale, &classicui->font);
    GetValidFont(classicui->strUserLocale, &classicui->menuFont);
#endif

    FcitxXlibWindowPaint(&classicui->mainWindow->parent);
    FcitxXlibWindowPaint(&classicui->inputWindow->parent);
    TrayWindowDraw(classicui->trayWindow);

    SaveClassicUIConfig(classicui);

    classicui->epoch ++;
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
    UT_array* skinNameBuf = &classicui->skinNameBuf;
    utarray_clear(skinBuf);
    utarray_clear(skinNameBuf);
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
                    // For reading skin name.
                    FcitxSkin tempSkin;
                    char* skinName = strdup(drt->d_name);
                    memset(&tempSkin, 0, sizeof(tempSkin));
                    if (LoadSkinConfig(&tempSkin, &skinName, /*fallback=*/false) == 0) {
                        if (fcitx_utf8_check_string(tempSkin.skinInfo.skinName)) {
                            char *temp = drt->d_name;
                            char *tempName = tempSkin.skinInfo.skinName;
                            utarray_push_back(skinBuf, &temp);
                            utarray_push_back(skinNameBuf, &tempName);
                        }
                    }
                    fcitx_utils_free(skinName);

                    utarray_done(&tempSkin.skinMainBar.skinPlacement);
                    FcitxConfigFree(&tempSkin.config);
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
    utarray_init(&classicui->skinNameBuf, fcitx_str_icd);
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
    if (shell) {
        char** name = (char**) utarray_eltptr(&classicui->skinBuf, index);
        DisplaySkin(classicui, *name);
    }
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
        char** name = (char**) utarray_eltptr(&classicui->skinNameBuf, i);
        FcitxMenuAddMenuItem(menu, *name, MENUTYPE_SIMPLE, NULL);
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
            activeIcon ? cairo_image_surface_get_width(activeIcon->image) : 22,
            activeIcon ? cairo_image_surface_get_height(activeIcon->image) : 22, true);
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
