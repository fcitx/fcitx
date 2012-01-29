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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/
#include <limits.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <cairo.h>
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx/module.h"
#include <module/x11/x11stuff.h>

#include "fcitx/ime.h"
#include "fcitx/instance.h"
#include "fcitx-utils/log.h"
#include "fcitx/frontend.h"
#include "fcitx-config/xdg.h"
#include <cairo-xlib.h>
#include "ui/cairostuff/cairostuff.h"
#include "ui/cairostuff/font.h"
#include "ui/classic/classicuiinterface.h"
#include "fcitx/hook.h"
#include "fcitx-utils/utils.h"

#define VK_FILE "vk.conf"

#define VK_WINDOW_WIDTH     354
#define VK_WINDOW_HEIGHT    164
#define VK_NUMBERS      47
#define VK_MAX          50

struct _FcitxVKState;

typedef struct _VKS {
    char            strSymbol[VK_NUMBERS][2][UTF8_MAX_LENGTH + 1]; //相应的符号
    char           *strName;
} VKS;

typedef struct _VKWindow {
    Window          window;
    FcitxConfigColor* fontColor;
    int fontSize;
    cairo_surface_t* surface;
    cairo_surface_t* keyboard;
    Display*        dpy;
    struct _FcitxVKState* owner;
    char **font;
    char *defaultFont;
    int iVKWindowX;
    int iVKWindowY;
} VKWindow;

typedef struct _FcitxVKState {
    VKWindow*       vkWindow;
    int             iCurrentVK ;
    int             iVKCount ;
    VKS             vks[VK_MAX];
    boolean         bShiftPressed;
    boolean         bVKCaps;
    boolean         bVK;
    FcitxUIMenu     vkmenu;
    FcitxInstance* owner;
    FcitxAddon* classicui;
} FcitxVKState;

const char            vkTable[VK_NUMBERS + 1] = "`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./";
const char            strCharTable[] = "`~1!2@3#4$5%6^7&8*9(0)-_=+[{]}\\|;:'\",<.>/?";    //用于转换上/下档键

static boolean VKWindowEventHandler(void* arg, XEvent* event);
static void
VKInitWindowAttribute(FcitxVKState* vkstate, Visual ** vs, Colormap * cmap,
                      XSetWindowAttributes * attrib,
                      unsigned long *attribmask, int *depth);
static Visual * VKFindARGBVisual(FcitxVKState* vkstate);
static void VKSetWindowProperty(FcitxVKState* vkstate, Window window, FcitxXWindowType type, char *windowTitle);
static boolean VKMouseClick(FcitxVKState* vkstate, Window window, int *x, int *y);
static void SwitchVK(FcitxVKState *vkstate);
static void LoadVKMapFile(FcitxVKState *vkstate);
static void ChangVK(FcitxVKState* vkstate);
static void ReloadVK(void *arg);
static int MyToUpper(int iChar);
static int MyToLower(int iChar);
static cairo_surface_t* LoadVKImage(VKWindow* vkWindow);
static void *VKCreate(FcitxInstance* instance);
static VKWindow* CreateVKWindow(FcitxVKState* vkstate);
static boolean GetVKState(void *arg);
static void ToggleVKState(void *arg);
static INPUT_RETURN_VALUE ToggleVKStateWithHotkey(void* arg);
static void DrawVKWindow(VKWindow* vkWindow);
static boolean VKMouseKey(FcitxVKState* vkstate, int x, int y);
static boolean VKPreFilter(void* arg, FcitxKeySym sym,
                           unsigned int state,
                           INPUT_RETURN_VALUE *retval
                          );
static  void VKReset(void* arg);
static void VKUpdate(void* arg);
static INPUT_RETURN_VALUE DoVKInput(FcitxVKState* vkstate, KeySym sym, int state);
static void DisplayVKWindow(VKWindow* vkWindow);
static boolean VKMenuAction(FcitxUIMenu *menu, int index);
static void UpdateVKMenu(FcitxUIMenu *menu);
static void SelectVK(FcitxVKState* vkstate, int vkidx);

static FcitxConfigColor blackColor = {0, 0, 0};

FCITX_EXPORT_API
FcitxModule module = {
    VKCreate,
    NULL,
    NULL,
    NULL,
    ReloadVK
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void *VKCreate(FcitxInstance* instance)
{
    FcitxVKState *vkstate = fcitx_utils_malloc0(sizeof(FcitxVKState));
    FcitxGlobalConfig* config = FcitxInstanceGetGlobalConfig(instance);
    vkstate->owner = instance;
    vkstate->classicui = FcitxAddonsGetAddonByName(FcitxInstanceGetAddons(instance), FCITX_CLASSIC_UI_NAME);

    FcitxHotkeyHook hotkey;
    hotkey.hotkey = config->hkVK;
    hotkey.hotkeyhandle = ToggleVKStateWithHotkey;
    hotkey.arg = vkstate;
    FcitxInstanceRegisterHotkeyFilter(instance, hotkey);

    FcitxUIRegisterStatus(instance, vkstate, "vk", _("Virtual Keyboard"), _("Virtual Keyboard State"),  ToggleVKState, GetVKState);

    LoadVKMapFile(vkstate);

    FcitxKeyFilterHook hk;
    hk.arg = vkstate ;
    hk.func = VKPreFilter;
    FcitxInstanceRegisterPreInputFilter(instance, hk);

    FcitxIMEventHook resethk;
    resethk.arg = vkstate;
    resethk.func = VKReset;
    FcitxInstanceRegisterTriggerOnHook(instance, resethk);
    FcitxInstanceRegisterTriggerOffHook(instance, resethk);

    resethk.func = VKUpdate;
    FcitxInstanceRegisterInputFocusHook(instance, resethk);
    FcitxInstanceRegisterInputUnFocusHook(instance, resethk);

    FcitxMenuInit(&vkstate->vkmenu);
    vkstate->vkmenu.candStatusBind = strdup("vk");
    vkstate->vkmenu.name = strdup(_("Virtual Keyboard"));

    vkstate->vkmenu.UpdateMenu = UpdateVKMenu;
    vkstate->vkmenu.MenuAction = VKMenuAction;
    vkstate->vkmenu.priv = vkstate;
    vkstate->vkmenu.isSubMenu = false;

    int i;
    for (i = 0; i < vkstate->iVKCount; i ++)
        FcitxMenuAddMenuItem(&vkstate->vkmenu, vkstate->vks[i].strName, MENUTYPE_SIMPLE, NULL);

    FcitxUIRegisterMenu(instance, &vkstate->vkmenu);

    return vkstate;
}

boolean VKMenuAction(FcitxUIMenu *menu, int index)
{
    FcitxVKState* vkstate = (FcitxVKState*) menu->priv;
    SelectVK(vkstate, index);
    return true;
}

void UpdateVKMenu(FcitxUIMenu *menu)
{
    FcitxVKState* vkstate = (FcitxVKState*) menu->priv;
    menu->mark = vkstate->iCurrentVK;
}

void VKReset(void* arg)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    VKWindow* vkWindow = vkstate->vkWindow;
    if (vkstate->bVK != false)
        FcitxUIUpdateStatus(vkstate->owner, "vk");
    if (vkWindow)
        XUnmapWindow(vkWindow->dpy, vkWindow->window);
}

void VKUpdate(void* arg)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    VKWindow* vkWindow = vkstate->vkWindow;
    if (vkWindow) {

        if (FcitxInstanceGetCurrentState(vkstate->owner) != IS_CLOSED && vkstate->bVK) {
            DrawVKWindow(vkWindow);
            DisplayVKWindow(vkWindow);
        } else
            XUnmapWindow(vkWindow->dpy, vkWindow->window);
    }
}

boolean VKPreFilter(void* arg, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    if (vkstate->bVK) {
        INPUT_RETURN_VALUE ret = DoVKInput(vkstate, sym, state);
        *retval = ret;
        return true;
    }
    return false;
}

boolean GetVKState(void *arg)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    return vkstate->bVK;
}

void ToggleVKState(void *arg)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    SwitchVK(vkstate);
}

INPUT_RETURN_VALUE ToggleVKStateWithHotkey(void* arg)
{
    FcitxVKState *vkstate = (FcitxVKState*) arg;
    FcitxUIUpdateStatus(vkstate->owner, "vk");
    return IRV_DO_NOTHING;
}

VKWindow* CreateVKWindow(FcitxVKState* vkstate)
{
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    char        strWindowName[] = "Fcitx VK Window";
    Colormap cmap;
    Visual * vs;
    int depth;
    FcitxModuleFunctionArg arg;
    VKWindow* vkWindow = fcitx_utils_malloc0(sizeof(VKWindow));
    vkWindow->owner = vkstate;

    LoadVKImage(vkWindow);

    vs = VKFindARGBVisual(vkstate);
    VKInitWindowAttribute(vkstate, &vs, &cmap, &attrib, &attribmask, &depth);
    vkWindow->dpy = InvokeFunction(vkstate->owner, FCITX_X11, GETDISPLAY, arg);

    vkWindow->fontSize = 12;
    if (vkstate->classicui) {
        FcitxModuleFunctionArg arg;
        vkWindow->fontColor = InvokeFunction(vkstate->owner, FCITX_CLASSIC_UI, GETKEYBOARDFONTCOLOR, arg);
        vkWindow->font = InvokeFunction(vkstate->owner, FCITX_CLASSIC_UI, GETFONT, arg);
    } else {
        vkWindow->fontColor = &blackColor;
        vkWindow->defaultFont = strdup("sans");
#ifndef _ENABLE_PANGO
        GetValidFont("zh", &vkWindow->defaultFont);
#endif
        vkWindow->font = &vkWindow->defaultFont;
    }

    vkWindow->window = XCreateWindow(vkWindow->dpy,
                                     DefaultRootWindow(vkWindow->dpy),
                                     0, 0,
                                     VK_WINDOW_WIDTH, VK_WINDOW_HEIGHT,
                                     0, depth, InputOutput, vs, attribmask, &attrib);
    if (vkWindow->window == (Window) None)
        return NULL;

    vkWindow->surface = cairo_xlib_surface_create(vkWindow->dpy, vkWindow->window, vs, VK_WINDOW_WIDTH, VK_WINDOW_HEIGHT);

    XSelectInput(vkWindow->dpy, vkWindow->window, ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask);

    VKSetWindowProperty(vkstate, vkWindow->window, FCITX_WINDOW_DOCK, strWindowName);

    FcitxModuleFunctionArg arg2;
    arg2.args[0] = VKWindowEventHandler;
    arg2.args[1] = vkWindow;
    InvokeFunction(vkstate->owner, FCITX_X11, ADDXEVENTHANDLER, arg2);

    return vkWindow;
}

boolean VKWindowEventHandler(void* arg, XEvent* event)
{
    VKWindow* vkWindow = arg;
    if (event->xany.window == vkWindow->window) {
        switch (event->type) {
        case Expose:
            DrawVKWindow(vkWindow);
            break;
        case ButtonPress:
            switch (event->xbutton.button) {
            case Button1: {
                if (!VKMouseKey(vkWindow->owner, event->xbutton.x, event->xbutton.y)) {
                    vkWindow->iVKWindowX = event->xbutton.x;
                    vkWindow->iVKWindowY = event->xbutton.y;
                    VKMouseClick(vkWindow->owner, vkWindow->window, &vkWindow->iVKWindowX, &vkWindow->iVKWindowY);
                    DrawVKWindow(vkWindow);
                }
            }
            break;
            }
            break;
        }
        return true;
    }

    return false;
}

cairo_surface_t* LoadVKImage(VKWindow* vkWindow)
{
    FcitxVKState* vkstate = vkWindow->owner;
    if (vkstate->classicui) {
        FcitxModuleFunctionArg arg;
        boolean fallback = true;
        char vkimage[] = "keyboard.png";
        arg.args[0] = vkimage;
        arg.args[1] = &fallback;
        return InvokeFunction(vkstate->owner, FCITX_CLASSIC_UI, LOADIMAGE, arg);
    } else {
        if (!vkWindow->keyboard) {
            char path[] = PKGDATADIR "/skin/default/keyboard.png";
            vkWindow->keyboard = cairo_image_surface_create_from_png(path);
        }
        return vkWindow->keyboard;
    }
    return NULL;
}

void DisplayVKWindow(VKWindow* vkWindow)
{
    XMapRaised(vkWindow->dpy, vkWindow->window);
}

void DestroyVKWindow(VKWindow* vkWindow)
{
    cairo_surface_destroy(vkWindow->surface);
    XDestroyWindow(vkWindow->dpy, vkWindow->window);
}

void DrawVKWindow(VKWindow* vkWindow)
{
    int             i;
    int             iPos;
    cairo_t *cr;
    FcitxVKState *vkstate = vkWindow->owner;
    VKS *vks = vkstate->vks;


    cr = cairo_create(vkWindow->surface);
    cairo_surface_t* vkimage = LoadVKImage(vkWindow);
    cairo_set_source_surface(cr, vkimage, 0, 0);
    cairo_paint(cr);
    /* 显示字符 */
    /* 名称 */
    OutputString(cr, vks[vkstate->iCurrentVK].strName, *vkWindow->font, vkWindow->fontSize , (VK_WINDOW_WIDTH - StringWidth(vks[vkstate->iCurrentVK].strName, *vkWindow->font, vkWindow->fontSize)) / 2, 6, vkWindow->fontColor);

    /* 第一排 */
    iPos = 13;
    for (i = 0; i < 13; i++) {
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][1], *vkWindow->font, vkWindow->fontSize, iPos, 27, vkWindow->fontColor);
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][0], *vkWindow->font, vkWindow->fontSize, iPos - 5, 40, vkWindow->fontColor);
        iPos += 24;
    }
    /* 第二排 */
    iPos = 48;
    for (i = 13; i < 26; i++) {
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][1], *vkWindow->font, vkWindow->fontSize, iPos, 55, vkWindow->fontColor);
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][0], *vkWindow->font, vkWindow->fontSize, iPos - 5, 68, vkWindow->fontColor);
        iPos += 24;
    }
    /* 第三排 */
    iPos = 55;
    for (i = 26; i < 37; i++) {
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][1], *vkWindow->font, vkWindow->fontSize, iPos, 83, vkWindow->fontColor);
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][0], *vkWindow->font, vkWindow->fontSize, iPos - 5, 96, vkWindow->fontColor);
        iPos += 24;
    }

    /* 第四排 */
    iPos = 72;
    for (i = 37; i < 47; i++) {
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][1], *vkWindow->font, vkWindow->fontSize, iPos, 111, vkWindow->fontColor);
        OutputString(cr, vks[vkstate->iCurrentVK].strSymbol[i][0], *vkWindow->font, vkWindow->fontSize, iPos - 5, 124, vkWindow->fontColor);
        iPos += 24;
    }

    cairo_destroy(cr);
}

/*
 * 处理相关鼠标键
 */
boolean VKMouseKey(FcitxVKState* vkstate, int x, int y)
{
    int             iIndex = 0;
    char            strKey[3] = { 0, 0, 0};
    char           *pstr = NULL;
    FcitxInstance* instance = vkstate->owner;

    if (FcitxUIIsInBox(x, y, 1, 1, VK_WINDOW_WIDTH, 16))
        ChangVK(vkstate);
    else {
        if (FcitxInstanceGetCurrentIC(instance) == NULL)
            return false;

        strKey[1] = '\0';
        pstr = strKey;
        if (y >= 28 && y <= 55) {   //第一行
            if (x < 4 || x > 348)
                return false;

            x -= 4;
            if (x >= 313 && x <= 344) { //backspace
                FcitxInstanceForwardKey(instance, FcitxInstanceGetCurrentIC(instance), FCITX_PRESS_KEY, FcitxKey_BackSpace, 0);
                return true;
            } else {
                iIndex = x / 24;
                if (iIndex > 12)    //避免出现错误
                    iIndex = 12;
                pstr = vkstate->vks[vkstate->iCurrentVK].strSymbol[iIndex][vkstate->bShiftPressed ^ vkstate->bVKCaps];
                if (vkstate->bShiftPressed) {
                    vkstate->bShiftPressed = false;
                    DrawVKWindow(vkstate->vkWindow);
                }
            }
        } else if (y >= 56 && y <= 83) { //第二行
            if (x < 4 || x > 350)
                return false;

            if (x >= 4 && x < 38) { //Tab
                FcitxInstanceForwardKey(instance, FcitxInstanceGetCurrentIC(instance), FCITX_PRESS_KEY, FcitxKey_Tab, 0);
                return true;
            } else {
                iIndex = 13 + (x - 38) / 24;
                pstr = vkstate->vks[vkstate->iCurrentVK].strSymbol[iIndex][vkstate->bShiftPressed ^ vkstate->bVKCaps];
                if (vkstate->bShiftPressed) {
                    vkstate->bShiftPressed = false;
                    DrawVKWindow(vkstate->vkWindow);
                }
            }
        } else if (y >= 84 && y <= 111) { //第三行
            if (x < 4 || x > 350)
                return false;

            if (x >= 4 && x < 44) { //Caps
                //改变大写键状态
                vkstate->bVKCaps = !vkstate->bVKCaps;
                pstr = (char *) NULL;
                DrawVKWindow(vkstate->vkWindow);
            } else if (x > 308 && x <= 350) //Return
                strKey[0] = '\n';
            else {
                iIndex = 26 + (x - 44) / 24;
                pstr = vkstate->vks[vkstate->iCurrentVK].strSymbol[iIndex][vkstate->bShiftPressed ^ vkstate->bVKCaps];
                if (vkstate->bShiftPressed) {
                    vkstate->bShiftPressed = false;
                    DrawVKWindow(vkstate->vkWindow);
                }
            }
        } else if (y >= 112 && y <= 139) {  //第四行
            if (x < 4 || x > 302)
                return false;

            if (x >= 4 && x < 62) { //SHIFT
                //改变SHIFT键状态
                vkstate->bShiftPressed = !vkstate->bShiftPressed;
                pstr = (char *) NULL;
                DrawVKWindow(vkstate->vkWindow);
            } else {
                iIndex = 37 + (x - 62) / 24;
                pstr = vkstate->vks[vkstate->iCurrentVK].strSymbol[iIndex][vkstate->bShiftPressed ^ vkstate->bVKCaps];
                if (vkstate->bShiftPressed) {
                    vkstate->bShiftPressed = false;
                    DrawVKWindow(vkstate->vkWindow);
                }
            }
        } else if (y >= 140 && y <= 162) {  //第五行
            if (x >= 4 && x < 38) { //Ins
                //改变INS键状态
                FcitxInstanceForwardKey(instance, FcitxInstanceGetCurrentIC(instance), FCITX_PRESS_KEY, FcitxKey_Insert, 0);
                return true;
            } else if (x >= 61 && x < 98) { //DEL
                FcitxInstanceForwardKey(instance, FcitxInstanceGetCurrentIC(instance), FCITX_PRESS_KEY, FcitxKey_Delete, 0);
                return true;
            } else if (x >= 99 && x < 270)  //空格
                strcpy(strKey, " ");
            else if (x >= 312 && x <= 350) {    //ESC
                SwitchVK(vkstate);
                pstr = (char *) NULL;
            } else
                return false;
        }

        if (pstr) {
            FcitxInstanceCommitString(instance, FcitxInstanceGetCurrentIC(instance), pstr);
            FcitxInstanceIncreateInputCharacterCount(instance, fcitx_utf8_strlen(pstr));
        }
    }

    return true;
}

/*
 * 读取虚拟键盘映射文件
 */
void LoadVKMapFile(FcitxVKState *vkstate)
{
    int             i, j;
    FILE           *fp;
    char           *buf = NULL;
    char           *pstr;
    VKS*            vks = vkstate->vks;
    size_t          len;

    for (j = 0; j < VK_MAX; j++) {
        for (i = 0; i < VK_NUMBERS; i++) {
            vks[j].strSymbol[i][0][0] = '\0';
            vks[j].strSymbol[i][1][0] = '\0';
        }
        if (vks[j].strName) {
            free(vks[j].strName);
            vks[j].strName = NULL;
        }
    }

    fp = FcitxXDGGetFileWithPrefix("data", VK_FILE, "rt", NULL);

    if (!fp)
        return;

    vkstate->iVKCount = 0;

    while (getline(&buf, &len, fp) != -1) {
        pstr = buf;
        while (*pstr == ' ' || *pstr == '\t')
            pstr++;
        if (pstr[0] == '#')
            continue;

        i = strlen(pstr) - 1;
        if (pstr[i] == '\n')
            pstr[i] = '\0';
        if (!strlen(pstr))
            continue;

        if (!strcmp(pstr, "[VK]"))
            vkstate->iVKCount++;
        else if (!strncmp(pstr, "NAME=", 5))
            vks[vkstate->iVKCount - 1].strName = strdup(pstr + 5);
        else {
            if (pstr[1] != '=' && !vkstate->iVKCount)
                continue;

            for (i = 0; i < VK_NUMBERS; i++) {
                if (vkTable[i] == tolower(pstr[0])) {
                    pstr += 2;
                    while (*pstr == ' ' || *pstr == '\t')
                        pstr++;

                    if (!(*pstr))
                        break;

                    j = 0;
                    while (*pstr && (*pstr != ' ' && *pstr != '\t'))
                        vks[vkstate->iVKCount - 1].strSymbol[i][0][j++] = *pstr++;
                    vks[vkstate->iVKCount - 1].strSymbol[i][0][j] = '\0';

                    j = 0;
                    while (*pstr == ' ' || *pstr == '\t')
                        pstr++;
                    if (*pstr) {
                        while (*pstr && (*pstr != ' ' && *pstr != '\t'))
                            vks[vkstate->iVKCount - 1].strSymbol[i][1][j++] = *pstr++;
                        vks[vkstate->iVKCount - 1].strSymbol[i][1][j] = '\0';
                    }

                    break;
                }
            }
        }
    }

    if (buf)
        free(buf);

    fclose(fp);
}

/*
 * 根据字符查找符号
 */
char           *VKGetSymbol(FcitxVKState *vkstate, char cChar)
{
    int             i;

    for (i = 0; i < VK_NUMBERS; i++) {
        if (MyToUpper(vkTable[i]) == cChar)
            return vkstate->vks[vkstate->iCurrentVK].strSymbol[i][1];
        if (MyToLower(vkTable[i]) == cChar)
            return vkstate->vks[vkstate->iCurrentVK].strSymbol[i][0];
    }

    return NULL;
}

/*
 * 上/下档键字符转换，以取代toupper和tolower
 */
int MyToUpper(int iChar)
{
    const char           *pstr;

    pstr = strCharTable;
    while (*pstr) {
        if (*pstr == iChar)
            return *(pstr + 1);
        pstr += 2;
    }

    return toupper(iChar);
}

int MyToLower(int iChar)
{
    const char           *pstr;

    pstr = strCharTable + 1;
    for (;;) {
        if (*pstr == iChar)
            return *(pstr - 1);
        if (!(*(pstr + 1)))
            break;
        pstr += 2;
    }

    return tolower(iChar);
}

void ChangVK(FcitxVKState* vkstate)
{
    vkstate->iCurrentVK++;
    if (vkstate->iCurrentVK == vkstate->iVKCount)
        vkstate->iCurrentVK = 0;

    vkstate->bVKCaps = false;
    vkstate->bShiftPressed = false;

    DrawVKWindow(vkstate->vkWindow);
}

INPUT_RETURN_VALUE DoVKInput(FcitxVKState* vkstate, KeySym sym, int state)
{
    char           *pstr = NULL;
    FcitxInputState *input = FcitxInstanceGetInputState(vkstate->owner);

    if (FcitxHotkeyIsHotKeySimple(sym, state))
        pstr = VKGetSymbol(vkstate, sym);
    if (!pstr)
        return IRV_TO_PROCESS;
    else {
        strcpy(FcitxInputStateGetOutputString(input), pstr);
        return IRV_COMMIT_STRING;
    }
}

void SwitchVK(FcitxVKState *vkstate)
{
    FcitxInstance* instance = vkstate->owner;
    if (vkstate->vkWindow == NULL)
        vkstate->vkWindow = CreateVKWindow(vkstate);
    VKWindow *vkWindow = vkstate->vkWindow;
    if (!vkstate->iVKCount)
        return;

    vkstate->bVK = !vkstate->bVK;

    if (vkstate->bVK) {
        int             x, y;
        int dwidth, dheight;
        FcitxModuleFunctionArg arg;
        arg.args[0] = &dwidth;
        arg.args[1] = &dheight;
        InvokeFunction(vkstate->owner, FCITX_X11, GETSCREENSIZE, arg);

        if (!FcitxUISupportMainWindow(instance)) {
            x = dwidth / 2 - VK_WINDOW_WIDTH / 2;
            y = 0;
        } else {
            int mx = 0, my = 0, mw = 0, mh = 0;
            FcitxUIGetMainWindowSize(instance, &mx, &my, &mw, &mh);
            x = mx;
            y = my + mh + 2;
            if ((y + VK_WINDOW_HEIGHT) >= dheight)
                y = my - VK_WINDOW_HEIGHT - 2;
            if (y < 0)
                y = 0;
        }
        if ((x + VK_WINDOW_WIDTH) >= dwidth)
            x = dwidth - VK_WINDOW_WIDTH - 1;
        if (x < 0)
            x = 0;


        XMoveWindow(vkWindow->dpy, vkWindow->window, x, y);
        DisplayVKWindow(vkWindow);
        FcitxUICloseInputWindow(instance);

        FcitxInputContext* ic = FcitxInstanceGetCurrentIC(instance);

        if (ic && FcitxInstanceGetCurrentState(instance) == IS_CLOSED)
            FcitxInstanceEnableIM(instance, ic, true);
    } else {
        XUnmapWindow(vkWindow->dpy, vkWindow->window);
        FcitxInstanceCleanInputWindow(instance);
        FcitxUIUpdateInputWindow(instance);
    }
}

/*
*选择指定index的虚拟键盘
*/
void SelectVK(FcitxVKState* vkstate, int vkidx)
{
    vkstate->bVK = false;
    vkstate->iCurrentVK = vkidx;
    FcitxUIUpdateStatus(vkstate->owner, "vk");
    if (vkstate->vkWindow)
        DrawVKWindow(vkstate->vkWindow);
}

void
VKInitWindowAttribute(FcitxVKState* vkstate, Visual ** vs, Colormap * cmap,
                      XSetWindowAttributes * attrib,
                      unsigned long *attribmask, int *depth)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = vs;
    arg.args[1] = cmap;
    arg.args[2] = attrib;
    arg.args[3] = attribmask;
    arg.args[4] = depth;
    InvokeFunction(vkstate->owner, FCITX_X11, INITWINDOWATTR, arg);
}

Visual * VKFindARGBVisual(FcitxVKState* vkstate)
{
    FcitxModuleFunctionArg arg;
    return InvokeFunction(vkstate->owner, FCITX_X11, FINDARGBVISUAL, arg);
}

void VKSetWindowProperty(FcitxVKState* vkstate, Window window, FcitxXWindowType type, char *windowTitle)
{
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = &type;
    arg.args[2] = windowTitle;
    InvokeFunction(vkstate->owner, FCITX_X11, SETWINDOWPROP, arg);
}

boolean
VKMouseClick(FcitxVKState* vkstate, Window window, int *x, int *y)
{
    boolean            bMoved = false;
    FcitxModuleFunctionArg arg;
    arg.args[0] = &window;
    arg.args[1] = x;
    arg.args[2] = y;
    arg.args[3] = &bMoved;
    InvokeFunction(vkstate->owner, FCITX_X11, MOUSECLICK, arg);

    return bMoved;
}

void ReloadVK(void* arg)
{
    FcitxVKState* vkstate = (FcitxVKState*)arg;
    LoadVKMapFile(vkstate);
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
