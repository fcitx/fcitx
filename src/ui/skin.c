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
#include <pthread.h>
#include <limits.h>

#include "core/fcitx.h"
#include "tools/tools.h"
#include "ui/ui.h"
#include "ui/skin.h"
#include "fcitx-config/xdg.h"
#include "tools/configfile.h"
#include "tools/profile.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/cutils.h"
#include "im/special/vk.h"
#ifdef _ENABLE_TRAY
#include "ui/TrayWindow.h"
#endif

#define ROUND_SIZE 60

static int LoadImage(FcitxImage * img,cairo_surface_t ** png, Bool fallback);
static void DestroyAllImage();
static ConfigFileDesc* GetSkinDesc();

//定义全局皮肤配置结构
FcitxSkin sc;

//指定皮肤类型 在config文件中配置
//指定皮肤所在的文件夹 一般在/usr/share/fcitx/skin目录下面
StringHashSet skinDir;

cairo_surface_t *  bar;
cairo_surface_t *  logo;
cairo_surface_t *  punc[2];
cairo_surface_t *  corner[2];
cairo_surface_t *  lx[2];
cairo_surface_t *  chs_t[2];
cairo_surface_t *  lock[2];
cairo_surface_t *  vk[2];
cairo_surface_t *  input;
cairo_surface_t *  menuBack;
cairo_surface_t *  prev;
cairo_surface_t *  next;
cairo_surface_t *  english;
cairo_surface_t *  otherim;
cairo_surface_t *  trayActive;
cairo_surface_t *  trayInactive;
cairo_surface_t *  keyBoard;

//定义全局皮肤配置结构
FcitxSkin sc;

MouseE ms_logo,ms_punc,ms_corner,ms_lx,ms_chs,ms_lock,ms_vk,ms_py;
MouseE *msE[] = {&ms_logo, &ms_punc, &ms_corner, &ms_lx, &ms_chs, &ms_lock, &ms_vk, &ms_py};
//指定皮肤类型 在config文件中配置
//指定皮肤所在的文件夹 一般在/usr/share/fcitx/skin目录下面
UT_array *skinBuf;
extern Display  *dpy;
static ConfigFileDesc * fcitxSkinDesc = NULL;

static ConfigFileDesc* GetSkinDesc()
{
    if (!fcitxSkinDesc)
    {
        FILE *tmpfp;
        tmpfp = GetXDGFileData("skin.desc", "r", NULL);
        fcitxSkinDesc = ParseConfigFileDescFp(tmpfp);
        fclose(tmpfp);
    }

    return fcitxSkinDesc;
}


/**
@加载皮肤配置文件
*/
int LoadSkinConfig()
{
    FILE    *fp;
    char  buf[PATH_MAX]={0};
    Bool    isreload = False;
    if (sc.config.configFile)
    {
        FreeConfigFile(sc.config.configFile);
        free(sc.skinInfo.skinName);
        free(sc.skinInfo.skinVersion);
        free(sc.skinInfo.skinAuthor);
        free(sc.skinInfo.skinDesc);
    }
    memset(&sc, 0, sizeof(FcitxSkin));

reload:
    //获取配置文件的绝对路径
    {
        if (!isreload)
        {
            snprintf(buf, PATH_MAX, "%s/fcitx_skin.conf", fc.skinType);
            buf[PATH_MAX-1] ='\0';
            size_t len;
            char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/skin" , DATADIR, "fcitx/skin" );

            fp = GetXDGFile(buf, path, "r", len, NULL);
            FreeXDGPath(path);
        }
        else
            fp = fopen(DATADIR "/fcitx/skin/default/fcitx_skin.conf", "r");
    }

    if (fp)
    {
        ConfigFile *cfile;
        ConfigFileDesc* skinDesc = GetSkinDesc();
        if (sc.config.configFile == NULL)
        {
            cfile = ParseConfigFileFp(fp, skinDesc);
        }
        else
        {
            cfile = sc.config.configFile;
            cfile = ParseIniFp(fp, cfile);
        }
        if (!cfile)
        {
            fclose(fp);
            fp = NULL;
        }
        else
        {
            FcitxSkinConfigBind(&sc, cfile, skinDesc);
            ConfigBindSync((GenericConfig*)&sc);
        }
    }

    if (!fp)
    {
        if (strcmp(fc.skinType, "default") == 0)
        {
            FcitxLog(FATAL, _("Can not load default skin, is installion correct?"));
            perror("fopen");
            exit(1);    // 如果安装目录里面也没有配置文件，那就只好告诉用户，无法运行了
        }
        perror("fopen");
        FcitxLog(WARNING, _("Can not load skin %s, return to default"), fc.skinType);
        if (fc.skinType)
            free(fc.skinType);
        fc.skinType = strdup("default");
        isreload = True;
        goto reload;
    }


    fclose(fp);

    return 0;

}

int LoadImage(FcitxImage * img,cairo_surface_t ** png, Bool fallback)
{
    char buf[PATH_MAX];
    if ( strlen(img->img_name) > 0 && strcmp( img->img_name ,"NONE.img") !=0)
    {
        char *skintype = strdup(fc.skinType);
        size_t len;
        char ** path = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/skin" , DATADIR, "fcitx/skin" );
        char *name;
        while (True) {
            snprintf(buf, PATH_MAX, "%s/%s", skintype, img->img_name);
            buf[PATH_MAX-1] ='\0';

            FILE* fp = GetXDGFile(buf, path, "r", len, &name);

            Bool flagNoFile = (fp == NULL);
            if (fp)
            {
                fclose(fp);

                *png=cairo_image_surface_create_from_png(name);
                if ( img->width ==0 || img->height== 0)
                {
                    img->width=cairo_image_surface_get_width(*png);
                    img->height=cairo_image_surface_get_height(*png);
                    img->response_w=img->width;
                    img->response_h=img->height;
                }
                break;
            }
            if (flagNoFile && (!fallback || strcmp(skintype, "default") == 0))
            {
                *png = NULL;
                break;
            }

            free(name);
            free(skintype);
            skintype = strdup("default");
        }
        free(name);
        free(skintype);
        FreeXDGPath(path);
    }
    return 0;
}


void LoadMainBarImage()
{
    LoadImage( &sc.skinMainBar.backImg, &bar , False);
    LoadImage( &sc.skinMainBar.logo, &logo, False);
    LoadImage( &sc.skinMainBar.zhpunc, &punc[0], False);
    LoadImage( &sc.skinMainBar.enpunc, &punc[1], False);
    LoadImage( &sc.skinMainBar.chs, &chs_t[0], False);
    LoadImage( &sc.skinMainBar.cht, &chs_t[1], False);
    LoadImage( &sc.skinMainBar.halfcorner, &corner[0], False);
    LoadImage( &sc.skinMainBar.fullcorner, &corner[1], False);
    LoadImage( &sc.skinMainBar.unlock, &lock[0], False);
    LoadImage( &sc.skinMainBar.lock, &lock[1], False);
    LoadImage( &sc.skinMainBar.nolegend, &lx[0], False);
    LoadImage( &sc.skinMainBar.legend, &lx[1], False);
    LoadImage( &sc.skinMainBar.novk, &vk[0], False);
    LoadImage( &sc.skinMainBar.vk, &vk[1], False);
    LoadImage( &sc.skinMainBar.eng, &english, False);
    LoadImage( &sc.skinMainBar.chn, &otherim, False);
    int i = 0;
    for (; i < iIMCount; i ++)
    {
        im[i].image.position_x = sc.skinMainBar.chn.position_x;
        im[i].image.position_y = sc.skinMainBar.chn.position_y;
        im[i].image.response_x = sc.skinMainBar.chn.response_x;
        im[i].image.response_y = sc.skinMainBar.chn.response_y;
        im[i].image.response_w = sc.skinMainBar.chn.response_w;
        im[i].image.response_h = sc.skinMainBar.chn.response_h;
        LoadImage(&im[i].image, &im[i].icon, False);
    }
}

void LoadVKImage()
{
    LoadImage( &sc.skinKeyboard.backImg, &keyBoard, True);
}

void DrawMenuBackground(XlibMenu * menu)
{
    int resizeHeight = sc.skinMenu.backImg.height - sc.skinMenu.marginTop - sc.skinMenu.marginBottom;
    int resizeWidth = sc.skinMenu.backImg.width - sc.skinMenu.marginLeft - sc.skinMenu.marginRight;
    int marginTop = sc.skinMenu.marginTop;
    int marginBottom = sc.skinMenu.marginBottom;
    int marginLeft = sc.skinMenu.marginLeft;
    int marginRight = sc.skinMenu.marginRight;

    if (resizeHeight <= 0)
        resizeHeight = 1;
    
    if (resizeWidth <= 0) 
        resizeWidth = 1;

    cairo_t *c = cairo_create(menu->menu_cs);
    
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(c, menuBack, 0, 0);
    
    
    /* 九宫格 
     * 7 8 9 
     * 4 5 6
     * 1 2 3
     */

    /* part 1 */
    cairo_save(c);
    cairo_translate(c, 0, menu->height - marginBottom);
    cairo_set_source_surface(c, menuBack, 0, -marginTop -resizeHeight);
    cairo_rectangle (c, 0, 0, marginLeft, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);
    
    /* part 3 */
    cairo_save(c);
    cairo_translate(c, menu->width - marginRight, menu->height - marginBottom);
    cairo_set_source_surface(c, menuBack, -marginLeft -resizeWidth, -marginTop -resizeHeight);
    cairo_rectangle (c, 0, 0, marginRight, marginBottom);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);

    
    /* part 7 */
    cairo_save(c);
    cairo_rectangle (c, 0, 0, marginLeft, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);
    
    /* part 9 */
    cairo_save(c);
    cairo_translate(c, menu->width - marginRight, 0);
    cairo_set_source_surface(c, menuBack, -marginLeft -resizeWidth, 0);
    cairo_rectangle (c, 0, 0, marginRight, marginTop);
    cairo_clip(c);
    cairo_paint(c);
    cairo_restore(c);
        
    /* part 2 & 8 */
    {
 
        if ( sc.skinMenu.resizeH == R_COPY)
        {
            int repaint_times=(menu->width - marginLeft - marginRight)/resizeWidth;
            int remain_width=(menu->width - marginLeft - marginRight)% resizeWidth;
            int i=0;

            for (i=0;i<repaint_times;i++)
            {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + i*resizeWidth, 0);
                cairo_set_source_surface(c, menuBack, -marginLeft, 0);
                cairo_rectangle (c,0,0,resizeWidth, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
                
                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  marginLeft + i*resizeWidth, menu->height - marginBottom);
                cairo_set_source_surface(c, menuBack,  -marginLeft, -marginTop -resizeHeight);
                cairo_rectangle (c,0,0,resizeWidth,marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_width != 0)
            {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, marginLeft + repaint_times*resizeWidth, 0);
                cairo_set_source_surface(c, menuBack, -marginLeft, 0);
                cairo_rectangle (c,0,0,remain_width, marginTop);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
                
                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  marginLeft + repaint_times*resizeWidth, menu->height - marginBottom);
                cairo_set_source_surface(c, menuBack,  -marginLeft, -marginTop -resizeHeight);
                cairo_rectangle (c,0,0,remain_width,marginBottom);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
        else if ( sc.skinMenu.resizeH == R_RESIZE)
        {
            cairo_save(c);
            cairo_translate(c, marginLeft, 0);
            cairo_scale(c, (double)(menu->width - marginLeft - marginRight)/(double)resizeWidth, 1);
            cairo_set_source_surface(c, menuBack, -marginLeft, 0);
            cairo_rectangle (c,0,0, resizeWidth, marginTop);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
                        
            cairo_save(c);
            cairo_translate(c, marginLeft, menu->height - marginBottom);
            cairo_scale(c, (double)(menu->width - marginLeft - marginRight)/(double)resizeWidth, 1);
            cairo_set_source_surface(c, menuBack, -marginLeft, -marginTop -resizeHeight);
            cairo_rectangle (c,0,0, resizeWidth, marginBottom);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
        }
    }
    
    /* part 4 & 6 */
    {
 
        if ( sc.skinMenu.resizeV == R_COPY)
        {
            int repaint_times=(menu->height - marginTop - marginBottom)/resizeHeight;
            int remain_height=(menu->height - marginTop - marginBottom)% resizeHeight;
            int i=0;

            for (i=0;i<repaint_times;i++)
            {
                /* part 4 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + i*resizeHeight);
                cairo_set_source_surface(c, menuBack, 0, -marginTop);
                cairo_rectangle (c,0,0, marginLeft, resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
                
                /* part 6 */
                cairo_save(c);
                cairo_translate(c, menu->width - marginRight,  marginTop + i*resizeHeight);
                cairo_set_source_surface(c, menuBack, -marginLeft -resizeWidth,  -marginTop);
                cairo_rectangle (c,0,0,marginRight,resizeHeight);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }

            if (remain_height != 0)
            {
                /* part 8 */
                cairo_save(c);
                cairo_translate(c, 0, marginTop + repaint_times*resizeHeight);
                cairo_set_source_surface(c, menuBack, 0, -marginTop);
                cairo_rectangle (c,0,0, marginLeft, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
                
                /* part 2 */
                cairo_save(c);
                cairo_translate(c,  menu->width - marginRight,  marginTop + repaint_times*resizeHeight);
                cairo_set_source_surface(c, menuBack, -marginLeft -resizeWidth,  -marginTop);
                cairo_rectangle (c,0,0,marginRight, remain_height);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
        else if ( sc.skinMenu.resizeV == R_RESIZE)
        {
            cairo_save(c);
            cairo_translate(c, 0, marginTop);
            cairo_scale(c, 1, (double)(menu->height - marginTop - marginBottom)/(double)resizeHeight);
            cairo_set_source_surface(c, menuBack, 0, -marginTop);
            cairo_rectangle (c,0,0, marginLeft, resizeHeight);
            cairo_clip(c);
            cairo_paint(c);
            cairo_restore(c);
                        
            cairo_save(c);
            cairo_translate(c, menu->width - marginRight, marginTop);
            cairo_scale(c, 1, (double)(menu->height - marginTop - marginBottom)/(double)resizeHeight);
            cairo_set_source_surface(c, menuBack, -marginLeft -resizeWidth, -marginTop);
            cairo_rectangle (c,0,0, marginRight, resizeHeight);
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
        
        if (sc.skinMenu.resizeH == R_COPY)
        {
            repaintH = (menu->width - marginLeft - marginRight)/resizeWidth + 1;
            remainW = (menu->width - marginLeft - marginRight)% resizeWidth;
        }
        else if (sc.skinMenu.resizeH == R_RESIZE)
        {
            repaintH = 1;
            scaleX = (double)(menu->width - marginLeft - marginRight)/(double)resizeWidth;
        }
        
        if (sc.skinMenu.resizeV == R_COPY)
        {            
            repaintV = (menu->height - marginTop - marginBottom)/(double)resizeHeight + 1;
            remainH = (menu->height - marginTop - marginBottom)%resizeHeight;
        }
        else if (sc.skinMenu.resizeV == R_RESIZE)
        {
            repaintV = 1;
            scaleY = (double)(menu->height - marginTop - marginBottom)/(double)resizeHeight;
        }
        

        int i, j;
        for (i = 0; i < repaintH; i ++)
        {
            for (j = 0; j < repaintV; j ++)
            {
                cairo_save(c);
                cairo_translate(c, marginLeft + i * resizeWidth , marginTop + j * resizeHeight);
                cairo_scale(c, scaleX, scaleY);
                cairo_set_source_surface(c, menuBack, -marginLeft, -marginTop);
                int w = resizeWidth,h = resizeHeight;

                if (sc.skinMenu.resizeV == R_COPY && j == repaintV - 1)
                    h = remainH;
                
                if (sc.skinMenu.resizeH == R_COPY && i == repaintH -1 )
                    w = remainW;
                
                cairo_rectangle (c,0,0, w, h);
                cairo_clip(c);
                cairo_paint(c);
                cairo_restore(c);
            }
        }
    }
    cairo_destroy(c);
}

void LoadInputBarImage()
{
    LoadImage( &sc.skinInputBar.backImg, &input, False);
    LoadImage( &sc.skinInputBar.backArrow, &prev, False);
    LoadImage( &sc.skinInputBar.forwardArrow, &next, False);
}

void LoadTrayImage()
{
    LoadImage( &sc.skinTrayIcon.active, &trayActive, False);
    LoadImage( &sc.skinTrayIcon.inactive, &trayInactive, False);
}

void LoadMenuImage()
{
    LoadImage( &sc.skinMenu.backImg, &menuBack, False);
}

void DestroyImage(cairo_surface_t ** png)
{
    if (png != NULL)
        cairo_surface_destroy(*png);
    *png=NULL;
}

void DestroyAllImage()
{
    DestroyImage(&bar);
    DestroyImage(&logo);
    DestroyImage(&punc[0]);
    DestroyImage(&punc[1]);
    DestroyImage(&corner[0]);
    DestroyImage(&corner[1]);
    DestroyImage(&lx[0]);
    DestroyImage(&lx[1]);
    DestroyImage(&chs_t[0]);
    DestroyImage(&chs_t[1]);
    DestroyImage(&lock[0]);
    DestroyImage(&lock[1]);
    DestroyImage(&vk[0]);
    DestroyImage(&vk[1]);
    DestroyImage(&english);
    DestroyImage(&otherim);

    DestroyImage(&input);
    DestroyImage(&prev);
    DestroyImage(&next);
    DestroyImage(&trayActive);
    DestroyImage(&trayInactive);

    DestroyImage(&menuBack);

    DestroyImage(&keyBoard);

    int i = 0;
    for (; i < iIMCount; i ++)
        DestroyImage(&im[i].icon);
}

/**
*输入条的绘制非常注重效率,画笔在绘图过程中不释放
*/
void LoadInputMessage()
{
    int i = 0;
    int fontSize;

    fontSize=sc.skinFont.fontSize;

    ConfigColor cursorColor = sc.skinInputBar.cursorColor;
    //输入条背景图画笔
    inputWindow.c_back = cairo_create(inputWindow.cs_input_bar);

    for (i = 0; i < 7 ; i ++)
    {
        inputWindow.c_font[i] = cairo_create(inputWindow.cs_input_bar);
        fcitx_cairo_set_color(inputWindow.c_font[i], &sc.skinFont.fontColor[i]);
#ifndef _ENABLE_PANGO
        SetFontContext(inputWindow.c_font[i], gs.font, sc.skinFont.fontSize);
#endif
    }
    inputWindow.c_font[7] = inputWindow.c_font[0];

    //光标画笔
    inputWindow.c_cursor=cairo_create(inputWindow.cs_input_bar);
    fcitx_cairo_set_color(inputWindow.c_cursor, &cursorColor);
    cairo_set_line_width (inputWindow.c_cursor, 1);
}

void DrawImage(cairo_t **c,FcitxImage img,cairo_surface_t * png,MouseE mouse)
{
    cairo_t * cr;
    if ( strlen(img.img_name) == 0 || strcmp( img.img_name ,"NONE.img") ==0 || !png)
        return;

    if (mouse == MOTION)
    {
        cairo_set_source_surface(*c,png, img.position_x, img.position_y);
        cairo_paint_with_alpha(*c,0.7);

    }
    else if (mouse == PRESS)
    {
        cr=cairo_create(mainWindow.cs_main_bar);
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        cairo_translate(cr, img.position_x+(int)(img.width*0.2/2), img.position_y+(int)(img.height*0.2/2));
        cairo_scale(cr, 0.8, 0.8);
        cairo_set_source_surface(cr,png,0,0);
        cairo_paint(cr);
        cairo_destroy(cr);
    }
    else //if( img.mouse == RELEASE)
    {
        cairo_set_source_surface(*c,png, img.position_x, img.position_y);
        cairo_paint(*c);
    }
}

void DrawInputBar(Messages * msgup, Messages *msgdown ,unsigned int * iwidth)
{
    int i;
    Bool bUseGBKT = fcitxProfile.bUseGBKT;
    char *strUp[MAX_MESSAGE_COUNT];
    char *strDown[MAX_MESSAGE_COUNT];
    int posUp[MAX_MESSAGE_COUNT];
    int posDown[MAX_MESSAGE_COUNT];
    int png_width,png_height;
    int repaint_times=0,remain_width=0;
    int input_bar_len=0, oldlen = *iwidth;
    int resizePos=0;
    int resizeWidth=0;
    RESIZERULE flag=0;
    int cursor_pos=0;
    int up_len,down_len;
    int iChar = iCursorPos;
    cairo_t *c;

    if (!msgup->changed && !msgdown->changed)
        return;

    resizePos=sc.skinInputBar.resizePos;
    resizeWidth=(sc.skinInputBar.resizeWidth==0)?20:sc.skinInputBar.resizeWidth;
    flag=sc.skinInputBar.resize;
    up_len = 0;
#ifdef _ENABLE_PANGO /* special case which only macro unable to handle */
    SetFontContext(dummy, gs.font, sc.skinFont.fontSize);
#endif

    for (i = 0; i < msgup->msgCount ; i++)
    {
        if (bUseGBKT)
            strUp[i] = ConvertGBKSimple2Tradition(msgup->msg[i].strMsg);
        else
            strUp[i] = msgup->msg[i].strMsg;
        posUp[i] = sc.skinInputBar.layoutLeft + up_len;

        up_len += StringWidthWithContext(inputWindow.c_font[msgup->msg[i].type], strUp[i]);
        if (inputWindow.bShowCursor)
        {
            int length = strlen(msgup->msg[i].strMsg);
            if (iChar >= 0)
            {
                if (iChar < length)
                {
                    char strTemp[MESSAGE_MAX_LENGTH];
                    char *strGBKT = NULL;
                    strncpy(strTemp, msgup->msg[i].strMsg, iChar);
                    strTemp[iChar] = '\0';
                    if (bUseGBKT)
                        strGBKT = ConvertGBKSimple2Tradition(strTemp);
                    else
                        strGBKT = strTemp;
                    cursor_pos= posUp[i]
                                + StringWidthWithContext(inputWindow.c_font[msgup->msg[i].type], strGBKT) + 2;
                    if (bUseGBKT)
                        free(strGBKT);
                }
                iChar -= length;
            }
        }

    }

    if (iChar >= 0)
        cursor_pos = sc.skinInputBar.layoutLeft + up_len;

    down_len = 0;
    for (i = 0; i < msgdown->msgCount ; i++)
    {
        if (bUseGBKT)
            strDown[i] = ConvertGBKSimple2Tradition(msgdown->msg[i].strMsg);
        else
            strDown[i] = msgdown->msg[i].strMsg;
        posDown[i] = sc.skinInputBar.layoutLeft + down_len;
        down_len += StringWidthWithContext(inputWindow.c_font[msgdown->msg[i].type], strDown[i]);
    }

    input_bar_len=(up_len<down_len)?down_len:up_len;
    input_bar_len+=sc.skinInputBar.layoutLeft+sc.skinInputBar.layoutRight;

    /* round to ROUND_SIZE in order to decrease resize */
    input_bar_len =  (input_bar_len / ROUND_SIZE) * ROUND_SIZE + ROUND_SIZE;

    //输入条长度应该比背景图长度要长,比最大长度要短
    input_bar_len=(input_bar_len>sc.skinInputBar.backImg.width)?input_bar_len:sc.skinInputBar.backImg.width;
    input_bar_len=(input_bar_len>=INPUT_BAR_MAX_LEN)?INPUT_BAR_MAX_LEN:input_bar_len;
    *iwidth=input_bar_len;

    png_width=sc.skinInputBar.backImg.width;
    png_height=sc.skinInputBar.backImg.height;

    //重绘的次数
    repaint_times=(input_bar_len - png_width)/resizeWidth+1;
    //不够汇一次的剩余的长度
    remain_width=(input_bar_len - png_width)%resizeWidth;

    if (oldlen != input_bar_len)
    {
        c=cairo_create(inputWindow.cs_input_back);

        //把cr设定位png图像,并保存
        cairo_set_source_surface(c, input, 0, 0);
        cairo_save(c);

        //画输入条第一部分(从开始到重复或者延伸段开始的位置)
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_rectangle (c,0,0,resizePos,png_height);
        cairo_clip(c);
        cairo_paint(c);

        //再画输入条的第三部分(因为第二部分可变,最后处理会比较方便)
        cairo_restore(c);
        cairo_save(c);
        cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
        cairo_translate(c, input_bar_len-(png_width-resizePos-resizeWidth), 0);
        cairo_set_source_surface(c, input, -resizePos-resizeWidth, 0);
        cairo_rectangle (c,0,0,png_width-resizePos-resizeWidth,png_height);
        cairo_clip(c);
        cairo_paint(c);

        //画第二部分,智能变化有两种方式
        if ( flag == R_COPY) //
        {
            int i=0;

            //先把整段的都绘上去
            for (i=0;i<repaint_times;i++)
            {
                cairo_restore(c);
                cairo_save(c);
                cairo_translate(c,resizePos+i*resizeWidth, 0);
                cairo_set_source_surface(c, input,-resizePos, 0);
                cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
                cairo_rectangle (c,0,0,resizeWidth,png_height);
                cairo_clip(c);
                cairo_paint(c);
            }

            //绘制剩余段
            if (remain_width != 0)
            {
                cairo_restore(c);
                cairo_translate(c,resizePos+resizeWidth*repaint_times, 0);
                cairo_set_source_surface(c, input,-resizePos, 0);
                cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
                cairo_rectangle (c,0,0,remain_width,png_height);
                cairo_clip(c);
                cairo_paint(c);
            }
        }
        else if ( flag == R_RESIZE)
        {
            cairo_restore(c);
            cairo_translate(c,resizePos, 0);
            cairo_scale(c, (double)(input_bar_len-png_width+resizeWidth)/(double)resizeWidth,1);
            cairo_set_source_surface(c, input,-resizePos, 0);
            cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
            cairo_rectangle (c,0,0,resizeWidth,png_height);
            cairo_clip(c);
            cairo_paint(c);
        }
        else
        {
            cairo_restore(c);
            cairo_translate(c,resizePos, 0);
            cairo_set_source_surface(c, input,-resizePos, 0);
            cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
            cairo_rectangle (c,0,0,resizeWidth,png_height);
            cairo_clip(c);
            cairo_paint(c);
        }
        cairo_destroy(c);
    }

    c = cairo_create(inputWindow.cs_input_bar);
    cairo_set_source_surface(c, inputWindow.cs_input_back, 0, 0);
    cairo_set_operator(c, CAIRO_OPERATOR_SOURCE);
    cairo_paint(c);

    cairo_set_operator(c, CAIRO_OPERATOR_OVER);

    if (inputWindow.bShowCursor )
    {
        //画向前向后箭头

        cairo_set_source_surface(inputWindow.c_back, prev,
                                 input_bar_len-(sc.skinInputBar.backImg.width-sc.skinInputBar.backArrow.position_x) ,
                                 sc.skinInputBar.backArrow.position_y);
        if (inputWindow.bShowPrev)
            cairo_paint(inputWindow.c_back);
        else
            cairo_paint_with_alpha(inputWindow.c_back,0.5);

        //画向前箭头
        cairo_set_source_surface(inputWindow.c_back, next,
                                 input_bar_len-(sc.skinInputBar.backImg.width-sc.skinInputBar.forwardArrow.position_x) ,
                                 sc.skinInputBar.forwardArrow.position_y);
        if (inputWindow.bShowNext)
            cairo_paint(inputWindow.c_back);
        else
            cairo_paint_with_alpha(inputWindow.c_back,0.5);
    }

    for (i = 0; i < msgup->msgCount ; i++)
    {
        OutputStringWithContext(inputWindow.c_font[msgup->msg[i].type], strUp[i], posUp[i], sc.skinInputBar.inputPos - sc.skinFont.fontSize);
        if (bUseGBKT) free(strUp[i]);
    }

    for (i = 0; i < msgdown->msgCount ; i++)
    {
        OutputStringWithContext(inputWindow.c_font[msgdown->msg[i].type], strDown[i], posDown[i], sc.skinInputBar.outputPos - sc.skinFont.fontSize);
        if (bUseGBKT) free(strDown[i]);
    }

    ResetFontContext();

    //画光标
    if (inputWindow.bShowCursor )
    {
        cairo_move_to(inputWindow.c_cursor,cursor_pos,sc.skinInputBar.inputPos+2);
        cairo_line_to(inputWindow.c_cursor,cursor_pos,sc.skinInputBar.inputPos-sc.skinFont.fontSize);
        cairo_stroke(inputWindow.c_cursor);
    }

    cairo_destroy(c);
    msgup->changed = False;
    msgdown->changed = False;
}

/*
*把鼠标状态初始化为某一种状态.
*/
Bool SetMouseStatus(MouseE m, MouseE* e, MouseE s)
{
    Bool changed = False;
    int i = 0;
    for (i = 0 ;i < 8; i ++)
    {
        MouseE obj;
        if (msE[i] == e)
            obj = s;
        else
            obj = m;
        
        if (obj != *msE[i])
            changed = True;
        
        *msE[i] = obj;
    }
    
    return changed;
}


void DisplaySkin(char * skinname)
{
    if (fc.bUseDBus)
        return;
    char *pivot = fc.skinType;
    fc.skinType= strdup(skinname);
    if (pivot)
        free(pivot);

    XUnmapWindow (dpy, mainWindow.window);
    CloseInputWindow();

    DestroyAllImage();

    DestroyMainWindow();
    DestroyInputWindow();
    DestroyMenuWindow();
    DestroyVKWindow();

    LoadSkinConfig();

    InitComposite();

    CreateMainWindow ();
    CreateInputWindow ();

    CreateMenuWindow();
    CreateVKWindow();

    DrawMainWindow ();
    DrawInputWindow ();
#ifdef _ENABLE_TRAY
    LoadTrayImage();
    if (GetCurrentState() == IS_CHN)
        DrawTrayWindow (ACTIVE_ICON, 0, 0, tray.size, tray.size );
    else
        DrawTrayWindow (INACTIVE_ICON, 0, 0, tray.size, tray.size );
#endif

    XMapRaised(dpy,mainWindow.window);
}

//图片文件加载函数完成
/*-------------------------------------------------------------------------------------------------------------*/
//skin目录下读入skin的文件夹名

int LoadSkinDirectory()
{
    if (!skinBuf)
    {
        skinBuf = malloc(sizeof(UT_array));
        utarray_init(skinBuf, &ut_str_icd);
    }
    else
    {
        utarray_clear(skinBuf);
    }
    int i ;
    DIR *dir;
    struct dirent *drt;
    struct stat fileStat;
    size_t len;
    char pathBuf[PATH_MAX];
    char **skinPath = GetXDGPath(&len, "XDG_CONFIG_HOME", ".config", "fcitx/skin" , DATADIR, "fcitx/skin" );
    for (i = 0; i< len; i++)
    {
        dir = opendir(skinPath[i]);
        if (dir == NULL)
            continue;

        while ((drt = readdir(dir)) != NULL)
        {
            if (strcmp(drt->d_name , ".") == 0 || strcmp(drt->d_name, "..") == 0)
                continue;
            sprintf(pathBuf,"%s/%s",skinPath[i],drt->d_name);

            if ( stat(pathBuf,&fileStat) == -1)
            {
                continue;
            }
            if ( fileStat.st_mode & S_IFDIR)
            {
                /* check duplicate name */
                int j = 0;
                for (;j<skinBuf->i;j++)
                {
                    char **name = (char**) utarray_eltptr(skinBuf, j);
                    if (strcmp(*name, drt->d_name) == 0)
                        break;
                }
                if (j == skinBuf->i)
                {
                    char *temp = drt->d_name;
                    utarray_push_back(skinBuf, &temp);
                }
            }
        }

        closedir(dir);
    }

    FreeXDGPath(skinPath);

    return 0;
}
