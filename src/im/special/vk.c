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
#include <limits.h>
#include <ctype.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <X11/Xatom.h>

#include "core/fcitx.h"
#include "im/special/vk.h"
#include "ui/ui.h"
#include "ui/MainWindow.h"
#include "ui/InputWindow.h"
#include "core/IC.h"
#include "core/xim.h"
#include "fcitx-config/xdg.h"
#include "interface/DBus.h"
#include "tools/profile.h"
#include "tools/configfile.h"

VKWindow vkWindow;

int             iVKWindowX = 0;
int             iVKWindowY = 0;
unsigned char   iCurrentVK = 0;
unsigned char   iVKCount = 0;

VKS             vks[VK_MAX];
char            vkTable[VK_NUMBERS + 1] = "`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./";

char            strCharTable[] = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:'\",<.>/?";    //用于转换上/下档键

Bool            bShiftPressed = False;
Bool            bVKCaps = False;

extern Display *dpy;
extern int      iScreen;

extern char     strStringGet[];
extern Bool     bVK;

extern XIMS     ims;

extern uint     iHZInputed;

#ifdef _ENABLE_DBUS
extern Property vk_prop;
#endif

char* sVKHotkey = NULL;

Bool CreateVKWindow (void)
{
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    char        strWindowName[] = "Fcitx VK Window";
    Colormap cmap;
    Visual * vs;
    int depth;

    LoadVKImage();

    vs = FindARGBVisual(dpy, iScreen);
    InitWindowAttribute(&vs, &cmap, &attrib, &attribmask, &depth);

    vkWindow.fontSize = 12;
    vkWindow.fontColor = sc.skinKeyboard.keyColor;

    vkWindow.window = XCreateWindow (dpy,
            DefaultRootWindow (dpy),
            0, 0,
            VK_WINDOW_WIDTH, VK_WINDOW_HEIGHT,
            0, depth, InputOutput, vs, attribmask, &attrib);
    if (vkWindow.window == (Window) NULL)
        return False;

    vkWindow.surface=cairo_xlib_surface_create(dpy, vkWindow.window, vs, VK_WINDOW_WIDTH, VK_WINDOW_HEIGHT);

    XSelectInput (dpy, vkWindow.window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask );

    XTextProperty   tp;
    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, vkWindow.window, &tp);

    LoadVKMapFile ();

    return True;
}

void DisplayVKWindow (void)
{
    XMapRaised (dpy, vkWindow.window);
}

void DestroyVKWindow (void)
{
    cairo_surface_destroy(vkWindow.surface);
    XDestroyWindow(dpy, vkWindow.window);
}

void DrawVKWindow (void)
{
    int             i;
    int             iPos;
    cairo_t *cr;

    cr=cairo_create(vkWindow.surface);
    cairo_set_source_surface(cr, keyBoard, 0, 0);
    cairo_paint(cr);
    /* 显示字符 */
    /* 名称 */
    OutputString (cr, vks[iCurrentVK].strName, gs.font, vkWindow.fontSize , (VK_WINDOW_WIDTH - StringWidth (vks[iCurrentVK].strName, gs.font, sc.skinFont.fontSize)) / 2, 6, &vkWindow.fontColor);

    /* 第一排 */
    iPos = 13;
    for (i = 0; i < 13; i++) {
        OutputString (cr, vks[iCurrentVK].strSymbol[i][1], gs.font, vkWindow.fontSize, iPos, 27, &vkWindow.fontColor);
        OutputString (cr, vks[iCurrentVK].strSymbol[i][0], gs.font, vkWindow.fontSize, iPos - 5, 40, &vkWindow.fontColor);
        iPos += 24;
    }
    /* 第二排 */
    iPos = 48;
    for (i = 13; i < 26; i++) {
        OutputString (cr, vks[iCurrentVK].strSymbol[i][1], gs.font, vkWindow.fontSize, iPos, 55, &vkWindow.fontColor);
        OutputString (cr, vks[iCurrentVK].strSymbol[i][0], gs.font, vkWindow.fontSize, iPos - 5, 68, &vkWindow.fontColor);
        iPos += 24;
    }
    /* 第三排 */
    iPos = 55;
    for (i = 26; i < 37; i++) {
        OutputString (cr, vks[iCurrentVK].strSymbol[i][1], gs.font, vkWindow.fontSize, iPos, 83, &vkWindow.fontColor);
        OutputString (cr, vks[iCurrentVK].strSymbol[i][0], gs.font, vkWindow.fontSize, iPos - 5, 96, &vkWindow.fontColor);
        iPos += 24;
    }

    /* 第四排 */
    iPos = 72;
    for (i = 37; i < 47; i++) {
        OutputString (cr, vks[iCurrentVK].strSymbol[i][1], gs.font, vkWindow.fontSize, iPos, 111, &vkWindow.fontColor);
        OutputString (cr, vks[iCurrentVK].strSymbol[i][0], gs.font, vkWindow.fontSize, iPos - 5, 124, &vkWindow.fontColor);
        iPos += 24;
    }

    cairo_destroy(cr);
}

/*
 * 处理相关鼠标键
 */
Bool VKMouseKey (int x, int y)
{
    int             iIndex;
    IMForwardEventStruct forwardEvent;
    char            strKey[3];
    char           *pstr;

    if (IsInBox (x, y, 1, 1, VK_WINDOW_WIDTH, 16))
        ChangVK ();
    else {
        if (!CurrentIC || !CurrentIC->id)
            return False;

        strKey[1] = '\0';
        pstr = strKey;
        if (y >= 28 && y <= 55) {   //第一行
            if (x < 4 || x > 348)
                return False;

            x -= 4;
            if (x >= 313 && x <= 344) { //backspace
                MyIMForwardEvent (CurrentIC->connect_id, CurrentIC->id, 22);
                return True;
            }
            else {
                iIndex = x / 24;
                if (iIndex > 12)    //避免出现错误
                    iIndex = 12;
                pstr = vks[iCurrentVK].strSymbol[iIndex][bShiftPressed ^ bVKCaps];
                if (bShiftPressed) {
                    bShiftPressed = False;
                    DrawVKWindow ();
                }
            }
        }
        else if (y >= 56 && y <= 83) {  //第二行
            if (x < 4 || x > 350)
                return False;

            if (x >= 4 && x < 38) { //Tab
                MyIMForwardEvent (CurrentIC->connect_id, CurrentIC->id, 23);
                return True;
            }
            else {
                iIndex = 13 + (x - 38) / 24;
                pstr = vks[iCurrentVK].strSymbol[iIndex][bShiftPressed ^ bVKCaps];
                if (bShiftPressed) {
                    bShiftPressed = False;
                    DrawVKWindow ();
                }
            }
        }
        else if (y >= 84 && y <= 111) { //第三行
            if (x < 4 || x > 350)
                return False;

            if (x >= 4 && x < 44) { //Caps
                //改变大写键状态
                bVKCaps = !bVKCaps;
                pstr = (char *) NULL;
                DrawVKWindow ();
            }
            else if (x > 308 && x <= 350)   //Return
                strKey[0] = '\n';
            else {
                iIndex = 26 + (x - 44) / 24;
                pstr = vks[iCurrentVK].strSymbol[iIndex][bShiftPressed ^ bVKCaps];
                if (bShiftPressed) {
                    bShiftPressed = False;
                    DrawVKWindow ();
                }
            }
        }
        else if (y >= 112 && y <= 139) {    //第四行
            if (x < 4 || x > 302)
                return False;

            if (x >= 4 && x < 62) { //SHIFT
                //改变SHIFT键状态
                bShiftPressed = !bShiftPressed;
                pstr = (char *) NULL;
                DrawVKWindow ();
            }
            else {
                iIndex = 37 + (x - 62) / 24;
                pstr = vks[iCurrentVK].strSymbol[iIndex][bShiftPressed ^ bVKCaps];
                if (bShiftPressed) {
                    bShiftPressed = False;
                    DrawVKWindow ();
                }
            }
        }
        else if (y >= 140 && y <= 162) {    //第五行
            if (x >= 4 && x < 38) { //Ins
                //改变INS键状态
                MyIMForwardEvent (CurrentIC->connect_id, CurrentIC->id, 106);
                return True;
            }
            else if (x >= 61 && x < 98) {   //DEL
                MyIMForwardEvent (CurrentIC->connect_id, CurrentIC->id, 107);
                return True;
            }
            else if (x >= 99 && x < 270)    //空格
                strcpy (strKey, "\xa1\xa1");
            else if (x >= 312 && x <= 350) {    //ESC
                SwitchVK ();
                pstr = (char *) NULL;
            }
            else
                return False;
        }

        if (pstr) {
            memset (&forwardEvent, 0, sizeof (IMForwardEventStruct));
            forwardEvent.connect_id = CurrentIC->connect_id;
            forwardEvent.icid = CurrentIC->id;
            SendHZtoClient (&forwardEvent, pstr);
            iHZInputed += (int) (utf8_strlen (pstr));   //粗略统计字数
        }
    }

    return True;
}

/*
 * 读取虚拟键盘映射文件
 */
void LoadVKMapFile (void)
{
    int             i, j;
    FILE           *fp;
    char            strPath[PATH_MAX];
    char           *pstr;

    for (j = 0; j < VK_MAX; j++) {
        for (i = 0; i < VK_NUMBERS; i++) {
            vks[j].strSymbol[i][0][0] = '\0';
            vks[j].strSymbol[i][1][0] = '\0';
        }
        vks[j].strName[0] = '\0';
    }

    fp = GetXDGFileData(VK_FILE, "rt", NULL);

    if (!fp)
        return;

    iVKCount = 0;

    for (;;) {
        if (!fgets (strPath, PATH_MAX, fp))
            break;
        pstr = strPath;
        while (*pstr == ' ' || *pstr == '\t')
            pstr++;
        if (pstr[0] == '#')
            continue;

        i = strlen (pstr) - 1;
        if (pstr[i] == '\n')
            pstr[i] = '\0';
        if (!strlen (pstr))
            continue;

        if (!strcmp (pstr, "[VK]"))
            iVKCount++;
        else if (!strncmp (pstr, "NAME=", 5))
            strcpy (vks[iVKCount - 1].strName, pstr + 5);
        else {
            if (pstr[1] != '=' && !iVKCount)
                continue;

            for (i = 0; i < VK_NUMBERS; i++) {
                if (vkTable[i] == tolower (pstr[0])) {
                    pstr += 2;
                    while (*pstr == ' ' || *pstr == '\t')
                        pstr++;

                    if (!(*pstr))
                        break;

                    j = 0;
                    while (*pstr && (*pstr != ' ' && *pstr != '\t'))
                        vks[iVKCount - 1].strSymbol[i][0][j++] = *pstr++;
                    vks[iVKCount - 1].strSymbol[i][0][j] = '\0';

                    j = 0;
                    while (*pstr == ' ' || *pstr == '\t')
                        pstr++;
                    if (*pstr) {
                        while (*pstr && (*pstr != ' ' && *pstr != '\t'))
                            vks[iVKCount - 1].strSymbol[i][1][j++] = *pstr++;
                        vks[iVKCount - 1].strSymbol[i][1][j] = '\0';
                    }

                    break;
                }
            }
        }
    }

    fclose (fp);
}

/*
 * 根据字符查找符号
 */
char           *VKGetSymbol (char cChar)
{
    int             i;

    for (i = 0; i < VK_NUMBERS; i++) {
        if (MyToUpper (vkTable[i]) == cChar)
            return vks[iCurrentVK].strSymbol[i][1];
        if (MyToLower (vkTable[i]) == cChar)
            return vks[iCurrentVK].strSymbol[i][0];
    }

    return NULL;
}

/*
 * 上/下档键字符转换，以取代toupper和tolower
 */
int MyToUpper (int iChar)
{
    char           *pstr;

    pstr = strCharTable;
    while (*pstr) {
        if (*pstr == iChar)
            return *(pstr + 1);
        pstr += 2;
    }

    return toupper (iChar);
}

int MyToLower (int iChar)
{
    char           *pstr;

    pstr = strCharTable + 1;
    for (;;) {
        if (*pstr == iChar)
            return *(pstr - 1);
        if (!(*(pstr + 1)))
            break;
        pstr += 2;
    }

    return tolower (iChar);
}

void ChangVK (void)
{
    iCurrentVK++;
    if (iCurrentVK == iVKCount)
        iCurrentVK = 0;

    bVKCaps = False;
    bShiftPressed = False;

    DrawVKWindow ();
    SwitchIM (-2);

    if (!fc.bUseDBus)
        DrawMainWindow ();
}

INPUT_RETURN_VALUE DoVKInput (KeySym sym, int state, int iCount)
{
    char           *pstr = NULL;

    if (IsHotKeySimple(sym, state))
        pstr = VKGetSymbol (sym);
    if (!pstr)
        return IRV_TO_PROCESS;
    else {
        strcpy (strStringGet, pstr);
        return IRV_GET_CANDWORDS;
    }
}

void SwitchVK (void)
{
    if (!iVKCount)
        return;

    bVK = !bVK;
    if (bVK) {
        int             x, y;
        int dwidth, dheight;
        GetScreenSize(&dwidth, &dheight);

        if (fc.bUseDBus)
            x = dwidth / 2 - VK_WINDOW_WIDTH / 2;
        else
            x = fcitxProfile.iMainWindowOffsetX;
        if ((x + VK_WINDOW_WIDTH) >= dwidth)
            x = dwidth - VK_WINDOW_WIDTH - 1;
        if (x < 0)
            x = 0;

        if (fc.bUseDBus)
            y = 0;
        else
            y = fcitxProfile.iMainWindowOffsetY + sc.skinMainBar.backImg.height + 2;
        if ((y + VK_WINDOW_HEIGHT) >= dheight)
            y = fcitxProfile.iMainWindowOffsetY - VK_WINDOW_HEIGHT - 2;
        if (y < 0)
            y = 0;

        XMoveWindow (dpy, vkWindow.window, x, y);
        DisplayVKWindow ();
        CloseInputWindow();

        if (CurrentIC && CurrentIC->state == IS_CLOSED)
            SetIMState (True);
    }
    else
        XUnmapWindow (dpy, vkWindow.window);

    SwitchIM (-2);

    if ( !fc.bUseDBus )
        DrawMainWindow ();
#ifdef _ENABLE_DBUS
    else
        updateProperty(&vk_prop);
#endif
}

/*
*选择指定index的虚拟键盘
*/
void SelectVK(int vkidx)
{
    bVK = False;
    iCurrentVK=vkidx;
    XUnmapWindow (dpy, vkWindow.window);
    SwitchVK();
}
