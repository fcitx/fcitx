/***************************************************************************
 *   Copyright (C) 2009~2010 by t3swing                                    *
 *   t3swing@sina.com                                                      *
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
#include <ctype.h>
#include <math.h>
#include <iconv.h>
#include <X11/Xatom.h>

#include "core/fcitx.h"
#include "ui/MenuWindow.h"
#include "ui/skin.h"
#include "im/special/vk.h"
#include "tools/configfile.h"

extern unsigned char iVKCount;
extern int      iScreen;
extern VKS             vks[];
extern Atom windowTypeAtom, typeMenuAtom;

XlibMenu mainMenu,imMenu,vkMenu,skinMenu;
static void DestroyXlibMenu(XlibMenu *menu);
static Bool CreateMainMenuWindow();
static Bool CreateImMenuWindow (void);
static Bool CreateSkinMenuWindow (void);
static Bool CreateVKMenuWindow();
static Bool ReverseColor(XlibMenu * Menu,int shellIndex);
static void MenuMark(Display * dpy,XlibMenu * Menu,int y,int i);
static void DrawArrow(cairo_t *cr, XlibMenu *menu, int line_y);
static void InitMenuDefault(XlibMenu * Menu);
static void AddMenuShell(XlibMenu* menu,char * tips,int isselect,MenuShellType type);
static void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth, int dwidth, int dheight);

const static UT_icd menuICD = {sizeof(MenuShell), NULL, NULL, NULL};

#define GetMenuShell(m, i) ((MenuShell*) utarray_eltptr(&(m)->shell, (i)))

void InitMenuDefault(XlibMenu * menu)
{
    menu->iPosX=100;
    menu->iPosY=100;
    menu->width=sc.skinMenu.backImg.width;
    menu->font_size=sc.skinFont.menuFontSize;
    strcpy(menu->font,gs.menuFont);
    menu->mark=-1;
    utarray_init(&menu->shell, &menuICD);
}

void AddMenuShell(XlibMenu* menu,char * tips,int isselect,MenuShellType type)
{
    MenuShell shell;
    strcpy(shell.tipstr,tips);
    shell.isselect = isselect;
    shell.type = type;
    shell.next = 0;

    utarray_push_back(&menu->shell, &shell);
}

int CreateXlibMenu(Display * dpy,XlibMenu * menu)
{
    char        strWindowName[]="Fcitx Menu Window";
    XSetWindowAttributes attrib;
    unsigned long   attribmask;
    int depth;
    int scr;
    Colormap cmap;
    Visual * vs;
    XGCValues xgv;
    GC gc;
    
    scr=DefaultScreen(dpy);
    vs=FindARGBVisual (dpy, scr);
    InitWindowAttribute(&vs, &cmap, &attrib, &attribmask, &depth);

    //开始只创建一个简单的窗口不做任何动作
    menu->menuWindow =XCreateWindow (dpy,
                                     RootWindow (dpy, scr),
                                     0, 0,
                                     MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT,
                                     0, depth, InputOutput,
                                     vs, attribmask, &attrib);

    if (menu->menuWindow == (Window) NULL)
        return False;

    XSetTransientForHint (dpy, menu->menuWindow, DefaultRootWindow (dpy));

    XChangeProperty (dpy, menu->menuWindow, windowTypeAtom, XA_ATOM, 32, PropModeReplace, (void *) &typeMenuAtom, 1);
    
    menu->pixmap = XCreatePixmap(dpy,
                                 menu->menuWindow,
                                 MENU_WINDOW_WIDTH,
                                 MENU_WINDOW_HEIGHT,
                                 depth);

    xgv.foreground = WhitePixel(dpy, scr);
    gc = XCreateGC(dpy, menu->pixmap, GCForeground, &xgv);
    XFillRectangle(
        dpy,
        menu->pixmap,
        gc,
        0,
        0,
        INPUT_BAR_MAX_LEN,
        inputWindow.iInputWindowHeight);
    menu->menu_cs=cairo_xlib_surface_create(dpy,
                                            menu->pixmap,
                                            vs,
                                            MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT);
    XFreeGC(dpy, gc);

    XSelectInput (dpy, menu->menuWindow, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask );

    XTextProperty   tp;
    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, menu->menuWindow, &tp);

    return True;
}

void GetMenuSize(Display * dpy,XlibMenu * menu)
{
    int i=0;
    int winheight=0;
    int fontheight=0;
    int menuwidth = 0;

    winheight = sc.skinMenu.marginTop + sc.skinMenu.marginBottom;//菜单头和尾都空8个pixel
    fontheight= menu->font_size;
    for (i=0;i<utarray_len(&menu->shell);i++)
    {
        if ( GetMenuShell(menu, i)->type == MENUSHELL)
            winheight += 6+fontheight;
        else if ( GetMenuShell(menu, i)->type == DIVLINE)
            winheight += 5;
        
        int width = StringWidth(GetMenuShell(menu, i)->tipstr, menu->font, menu->font_size);
        if (width > menuwidth)
            menuwidth = width;        
    }
    menu->height = winheight;
    menu->width = menuwidth + sc.skinMenu.marginLeft + sc.skinMenu.marginRight + 15 + 20;
}

//根据Menu内容来绘制菜单内容
void DrawXlibMenu(Display * dpy,XlibMenu * menu)
{
    GC gc = XCreateGC( dpy, menu->menuWindow, 0, NULL );
    int i=0;
    int fontheight;
    int iPosY = 0;

    fontheight= menu->font_size;

    GetMenuSize(dpy,menu);

    DrawMenuBackground(menu);

    iPosY=sc.skinMenu.marginTop;
    for (i=0;i<utarray_len(&menu->shell);i++)
    {
        if ( GetMenuShell(menu, i)->type == MENUSHELL)
        {
            DisplayText( dpy,menu,i,iPosY);
            if (menu->mark == i)//void menuMark(Display * dpy,xlibMenu * Menu,int y,int i)
                MenuMark(dpy,menu,iPosY,i);
            iPosY=iPosY+6+fontheight;
        }
        else if ( GetMenuShell(menu, i)->type == DIVLINE)
        {
            DrawDivLine(dpy,menu,iPosY);
            iPosY+=5;
        }
    }
    
    XResizeWindow(dpy, menu->menuWindow, menu->width, menu->height);
    XCopyArea (dpy,
               menu->pixmap,
               menu->menuWindow,
               gc,
               0,
               0,
               menu->width,
               menu->height, 0, 0);
    XFreeGC(dpy, gc);

}

void DisplayXlibMenu(Display * dpy,XlibMenu * menu)
{
    XMapRaised (dpy, menu->menuWindow);
    XMoveWindow(dpy, menu->menuWindow, menu->iPosX, menu->iPosY);
}

void DrawDivLine(Display * dpy,XlibMenu * Menu,int line_y)
{
    int marginLeft = sc.skinMenu.marginLeft;
    int marginRight = sc.skinMenu.marginRight;
    cairo_t * cr;
    cr=cairo_create(Menu->menu_cs);
    fcitx_cairo_set_color(cr, &sc.skinMenu.lineColor);
    cairo_set_line_width (cr, 2);
    cairo_move_to(cr, marginLeft + 3, line_y+3);
    cairo_line_to(cr, Menu->width - marginRight - 3, line_y+3);
    cairo_stroke(cr);
    cairo_destroy(cr);
}

void MenuMark(Display * dpy,XlibMenu * menu,int y,int i)
{
    int marginLeft = sc.skinMenu.marginLeft;
    double size = (menu->font_size * 0.7 ) / 2;
    cairo_t *cr;
    cr = cairo_create(menu->menu_cs);
    if (GetMenuShell(menu, i)->isselect == 0)
    {
        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_INACTIVE]);
    }
    else
    {
        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_translate(cr, marginLeft + 7, y + (menu->font_size / 2.0) );
    cairo_arc(cr, 0, 0, size , 0., 2*M_PI);
    cairo_fill(cr);
    cairo_destroy(cr);
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void DisplayText(Display * dpy,XlibMenu * menu,int shellindex,int line_y)
{
    int marginLeft = sc.skinMenu.marginLeft;
    int marginRight = sc.skinMenu.marginRight;
    cairo_t *  cr;
    cr=cairo_create(menu->menu_cs);

    SetFontContext(cr, menu->font, menu->font_size);

    if (GetMenuShell(menu, shellindex)->isselect ==0)
    {
        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_INACTIVE]);

        OutputStringWithContext(cr, GetMenuShell(menu, shellindex)->tipstr , 15 + marginLeft ,line_y);

        if (GetMenuShell(menu, shellindex)->next == 1)
            DrawArrow(cr, menu, line_y);
    }
    else
    {
        cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
        fcitx_cairo_set_color(cr, &sc.skinMenu.activeColor);
        cairo_rectangle (cr, marginLeft ,line_y, menu->width - marginRight - marginLeft,menu->font_size+4);
        cairo_fill (cr);

        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_ACTIVE]);
        OutputStringWithContext(cr, GetMenuShell(menu, shellindex)->tipstr , 15 + marginLeft ,line_y);

        if (GetMenuShell(menu, shellindex)->next == 1)
            DrawArrow(cr, menu, line_y);
    }
    ResetFontContext();
    cairo_destroy(cr);
}

void DrawArrow(cairo_t *cr, XlibMenu *menu, int line_y)
{
    int marginRight = sc.skinMenu.marginRight;
    double size = menu->font_size * 0.4;
    double offset = (menu->font_size - size) / 2;
    cairo_move_to(cr,menu->width - marginRight - 1 - size, line_y + offset);
    cairo_line_to(cr,menu->width - marginRight - 1 - size, line_y+size * 2 + offset);
    cairo_line_to(cr,menu->width - marginRight - 1, line_y + size + offset );
    cairo_line_to(cr,menu->width - marginRight - 1 - size ,line_y + offset);
    cairo_fill (cr);
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int SelectShellIndex(XlibMenu * menu, int x, int y, int* offseth)
{
    int i;
    int winheight=sc.skinMenu.marginTop;
    int fontheight;
    int marginLeft = sc.skinMenu.marginLeft;

    if (x < marginLeft)
        return -1;

    fontheight= menu->font_size;
    for (i=0;i<utarray_len(&menu->shell);i++)
    {
        if (GetMenuShell(menu, i)->type == MENUSHELL)
        {
            if (y>winheight+1 && y<winheight+6+fontheight-1)
            {
                if (offseth)
                    *offseth = winheight;
                return i;
            }
            winheight=winheight+6+fontheight;
        }
        else if (GetMenuShell(menu, i)->type == DIVLINE)
            winheight+=5;
    }
    return -1;
}

Bool ReverseColor(XlibMenu * menu,int shellIndex)
{
    Bool flag = False;
    int i;

    int last = -1;

    for (i=0;i<utarray_len(&menu->shell);i++)
    {
        if (GetMenuShell(menu, i)->isselect)
            last = i;

        GetMenuShell(menu, i)->isselect=0;
    }
    if (shellIndex == last)
        flag = True;
    if (shellIndex >=0 && shellIndex < utarray_len(&menu->shell))
        GetMenuShell(menu, shellIndex)->isselect = 1;
    return flag;
}

void ClearSelectFlag(XlibMenu * menu)
{
    int i;
    for (i=0;i< utarray_len(&menu->shell);i++)
    {
        GetMenuShell(menu, i)->isselect=0;
    }
}


//以上为菜单的简单封装，下面为对菜单的操作部分
//=========================================================================================
//创建菜单窗口
Bool CreateMenuWindow( )
{
    LoadMenuImage();
    CreateMainMenuWindow();
    CreateImMenuWindow();   //创建输入法选择菜单窗口
    CreateSkinMenuWindow(); //创建皮肤选择菜单窗口
    CreateVKMenuWindow();   //创建软键盘布局选择菜单窗口
    return 0;
}

void DestroyMenuWindow()
{
    DestroyXlibMenu(&mainMenu);
    DestroyXlibMenu(&imMenu);
    DestroyXlibMenu(&skinMenu);
    DestroyXlibMenu(&vkMenu);
}

void DestroyXlibMenu(XlibMenu *menu)
{
    cairo_surface_destroy(menu->menu_cs);
    XFreePixmap(dpy, menu->pixmap);
    XDestroyWindow(dpy, menu->menuWindow);
    utarray_done(&menu->shell);
}

Bool CreateMainMenuWindow()
{
    InitMenuDefault(&mainMenu);
    AddMenuShell(&mainMenu,_("About Fcitx"),0,MENUSHELL);
    AddMenuShell(&mainMenu,"",0,DIVLINE);
    AddMenuShell(&mainMenu,_("Switch Skin"),0,MENUSHELL);
    AddMenuShell(&mainMenu,_("Switch IM"),0,MENUSHELL);
    AddMenuShell(&mainMenu,_("Switch VK"),0,MENUSHELL);
    AddMenuShell(&mainMenu,"",0,DIVLINE);
    AddMenuShell(&mainMenu,_("Fcitx Config ..."),0,MENUSHELL);
    AddMenuShell(&mainMenu,_("Exit Fcitx"),0,MENUSHELL);
    GetMenuShell(&mainMenu, 2)->next=1;
    GetMenuShell(&mainMenu, 3)->next=1;
    GetMenuShell(&mainMenu, 4)->next=1;
    CreateXlibMenu(dpy,&mainMenu);
    GetMenuSize(dpy, &mainMenu);
    return 0;
}

//创建输入法选择菜单窗口
Bool CreateImMenuWindow()
{
    Bool ret;

    InitMenuDefault(&imMenu);
    ret=CreateXlibMenu(dpy,&imMenu);
    return ret;
}

//创建皮肤选择菜单窗口,皮肤菜单由于在窗口创建之初,信息不全,菜单结构在菜单显示之前再填充
Bool CreateSkinMenuWindow()
{
    Bool ret;
    InitMenuDefault(&skinMenu);
    ret=CreateXlibMenu(dpy,&skinMenu);
    return ret;
}

//创建软键盘布局选择菜单窗口
Bool CreateVKMenuWindow()
{
    Bool ret;
    int i;
    InitMenuDefault(&vkMenu);
    for(i = 0; i < iVKCount; i ++)
        AddMenuShell(&vkMenu, vks[i].strName, 0, MENUSHELL);

    ret=CreateXlibMenu(dpy,&vkMenu);
    return ret;

}

void MoveSubMenu(XlibMenu *sub, XlibMenu *parent, int offseth, int dwidth, int dheight)
{
    sub->iPosX=parent->iPosX + parent->width - sc.skinMenu.marginRight - 4;
    sub->iPosY=parent->iPosY + offseth - sc.skinMenu.marginTop;

    if ( sub->iPosX + sub->width > dwidth)
        sub->iPosX=parent->iPosX - sub->width + sc.skinMenu.marginLeft + 4;

    if ( sub->iPosY + sub->height > dheight)
        sub->iPosY = dheight - sub->height;
    
    XMoveWindow(dpy, sub->menuWindow, sub->iPosX, sub->iPosY);
}

//主菜单事件处理
void MainMenuEvent(int x,int y)
{
    int i,j;
    char tmpstr[64]={0};
    int dwidth, dheight;
    int offseth;
    GetScreenSize(&dwidth, &dheight);
    i=SelectShellIndex(&mainMenu, x, y, &offseth);
    Bool flag = ReverseColor(&mainMenu,i);
    if (flag)
        return;

    DrawXlibMenu( dpy,&mainMenu);
    DisplayXlibMenu(dpy,&mainMenu);

    switch (i)
    {
        //显示皮肤菜单
    case 2:
        utarray_clear(&skinMenu.shell);
        for (j=0;j<skinBuf->i;j++)
        {
            char **sskin = (char**)utarray_eltptr(skinBuf, j);
            AddMenuShell(&skinMenu, *sskin, 0, MENUSHELL);
        }
        ClearSelectFlag(&skinMenu);
        DrawXlibMenu(dpy,&skinMenu);
        MoveSubMenu(&skinMenu, &mainMenu, offseth, dwidth, dheight);
        DisplayXlibMenu(dpy,&skinMenu);
        break;
    case 3:
        utarray_clear(&imMenu.shell);
        for (i=0;i<iIMCount;i++)
        {
            memset(tmpstr,0,sizeof(tmpstr));
            sprintf(tmpstr,"%d.%s",i+1,_(im[i].strName));
            AddMenuShell(&imMenu,tmpstr,0,MENUSHELL);
        }

        ClearSelectFlag(&imMenu);
        DrawXlibMenu( dpy,&imMenu);
        MoveSubMenu(&imMenu, &mainMenu, offseth, dwidth, dheight);
        DisplayXlibMenu(dpy,&imMenu);
        break;
    case 4:
        ClearSelectFlag(&vkMenu)    ;
        DrawXlibMenu( dpy,&vkMenu);
        MoveSubMenu(&vkMenu, &mainMenu, offseth, dwidth, dheight);
        DisplayXlibMenu(dpy,&vkMenu);
        break;
    default :
        break;
    }

    if (GetMenuShell(&mainMenu, 2)->isselect == 0)
        XUnmapWindow (dpy, skinMenu.menuWindow);
    if (GetMenuShell(&mainMenu, 3)->isselect == 0)
        XUnmapWindow (dpy, imMenu.menuWindow);
    if (GetMenuShell(&mainMenu, 4)->isselect == 0)
        XUnmapWindow (dpy, vkMenu.menuWindow);
}

//输入法菜单事件处理
void IMMenuEvent(int x,int y)
{
    int i;
    i=SelectShellIndex(&imMenu,x, y, NULL);

    Bool flag = ReverseColor(&imMenu,i);
    if (flag)
        return;
    DrawXlibMenu(dpy,&imMenu);
    DisplayXlibMenu(dpy,&imMenu);
}

//虚拟键盘菜单事件处理
void VKMenuEvent(int x,int y)
{
    int i;
    i=SelectShellIndex(&vkMenu,x, y, NULL);
    Bool flag = ReverseColor(&vkMenu,i);

    if (flag)
        return;
    DrawXlibMenu(dpy,&vkMenu);
    DisplayXlibMenu(dpy,&vkMenu);
}

//皮肤菜单事件处理
void SkinMenuEvent(int x,int y)
{
    int i;
    i=SelectShellIndex(&skinMenu,x, y, NULL);
    Bool flag = ReverseColor(&skinMenu,i);
    if (flag)
        return;
    DrawXlibMenu(dpy,&skinMenu);
    DisplayXlibMenu(dpy,&skinMenu);
}

