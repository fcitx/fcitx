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

#include "MenuWindow.h"

#include <ctype.h>
#include <math.h>
#include <iconv.h>
#include <X11/Xatom.h>
#include "skin.h"
#include "fcitx-config/configfile.h"


extern int      iScreen;

xlibMenu mainMenu,imMenu,vkMenu,skinMenu; 
static void DestroyXlibMenu(xlibMenu *menu);
static Bool CreateMainMenuWindow();
static Bool CreateImMenuWindow (void);
static Bool CreateSkinMenuWindow (void);
static Bool CreateVKMenuWindow();

void InitMenuDefault(xlibMenu * Menu)
{
	Menu->pos_x=100;
	Menu->pos_y=100;
	Menu->width=sc.skinMenu.backImg.width;
	Menu->font_size=14;
	strcpy(Menu->font,gs.fontZh);
	Menu->mark=-1;
}

void SetMeueShell(menuShell * shell,char * tips,int isselect,shellType type)
{
	strcpy(shell->tipstr,tips);
	shell->isselect=isselect;
	shell->type=type;
}

int CreateXlibMenu(Display * dpy,xlibMenu * Menu)
{
    Atom            menu_wm_window_type = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom            type_menu = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE_MENU", False);
    char		strWindowName[]="Fcitx Menu Window";
	XSetWindowAttributes attrib;
	unsigned long   attribmask;
	int depth;
	int scr;
	Colormap cmap;
	Visual * vs;
	scr=DefaultScreen(dpy);
   	vs=find_argb_visual (dpy, scr);
    InitWindowAttribute(&vs, &cmap, &attrib, &attribmask, &depth);
	
    //开始只创建一个简单的窗口不做任何动作
    Menu->menuWindow =XCreateWindow (dpy,
            RootWindow (dpy, scr),
            0, 0,
            MENU_WINDOW_WIDTH,MENU_WINDOW_HEIGHT,
            0, depth, InputOutput,
            vs, attribmask, &attrib);

    if (Menu->menuWindow == (Window) NULL)
	{
		return False;
	}
	
    XSetTransientForHint (dpy, Menu->menuWindow, DefaultRootWindow (dpy));

    XChangeProperty (dpy, Menu->menuWindow, menu_wm_window_type, XA_ATOM, 32, PropModeReplace, (void *) &type_menu, 1);

	Menu->menu_cs=cairo_xlib_surface_create(dpy, Menu->menuWindow, vs, MENU_WINDOW_WIDTH, MENU_WINDOW_HEIGHT);
	
    XSelectInput (dpy, Menu->menuWindow, KeyPressMask | ExposureMask | ButtonPressMask | ButtonReleaseMask  | PointerMotionMask | LeaveWindowMask );

    XTextProperty	tp;
    /* Set the name of the window */
    tp.value = (void *)strWindowName;
    tp.encoding = XA_STRING;
    tp.format = 16;
    tp.nitems = strlen(strWindowName);
    XSetWMName (dpy, Menu->menuWindow, &tp);

    return True;
}

void GetMenuHeight(Display * dpy,xlibMenu * Menu)
{
	int i=0;
	int winheight=0;
	int fontheight=0;

	winheight = sc.skinMenu.marginTop + sc.skinMenu.marginBottom;//菜单头和尾都空8个pixel
	fontheight=FontHeight(Menu->font);
	for(i=0;i<Menu->useItemCount;i++)
	{
		if( Menu->shell[i].type == menushell)
			winheight += 6+fontheight;
		else if( Menu->shell[i].type == divline)
			winheight += 5;
	}
	Menu->height=winheight;
}

//根据Menu内容来绘制菜单内容
void DrawXlibMenu(Display * dpy,xlibMenu * Menu)
{
	int i=0;
	int fontheight;
    int pos_y = 0;

	fontheight=FontHeight(Menu->font);

	GetMenuHeight(dpy,Menu);

    draw_menu_background(Menu);

	pos_y=sc.skinMenu.marginTop;
	for (i=0;i<Menu->useItemCount;i++)
	{
		if( Menu->shell[i].type == menushell)
		{
			DisplayText( dpy,Menu,i,pos_y);
			if(Menu->mark == i)//void menuMark(Display * dpy,xlibMenu * Menu,int y,int i)
				menuMark(dpy,Menu,pos_y,i);
			pos_y=pos_y+6+fontheight;
		}
		else if( Menu->shell[i].type == divline)
		{
			DrawDivLine(dpy,Menu,pos_y);
			pos_y+=5;
		}
	}
    
	XMoveResizeWindow(dpy, Menu->menuWindow, Menu->pos_x,Menu->pos_y,Menu->width, Menu->height);
}

void DisplayXlibMenu(Display * dpy,xlibMenu * Menu)
{
	//clearSelectFlag(Menu);
	XMapRaised (dpy, Menu->menuWindow);	     
}

void DrawDivLine(Display * dpy,xlibMenu * Menu,int line_y)
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

void menuMark(Display * dpy,xlibMenu * Menu,int y,int i)
{
    int marginLeft = sc.skinMenu.marginLeft;
    cairo_t *cr;
    cr = cairo_create(Menu->menu_cs);
	if(Menu->shell[i].isselect == 0)
    {
        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_INACTIVE]);
    }
    else
    {
        fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_ACTIVE]);
    }
    cairo_translate(cr, marginLeft + 7, y + 10);
    cairo_arc(cr, 0, 0, 5.5 , 0., 2*M_PI);
    cairo_fill(cr);
	cairo_destroy(cr);
}

/*
* 显示菜单上面的文字信息,只需要指定窗口,窗口宽度,需要显示文字的上边界,字体,显示的字符串和是否选择(选择后反色)
* 其他都固定,如背景和文字反色不反色的颜色,反色框和字的位置等
*/
void DisplayText(Display * dpy,xlibMenu * Menu,int shellindex,int line_y)
{
    int marginLeft = sc.skinMenu.marginLeft;
    int marginRight = sc.skinMenu.marginRight;
	cairo_t *  cr;
	cr=cairo_create(Menu->menu_cs);
	if(Menu->shell[shellindex].isselect ==0)
	{
		fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_INACTIVE]);
		cairo_select_font_face(cr, Menu->font,CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, Menu->font_size);
	
        OutputStringWithContext(cr, Menu->shell[shellindex].tipstr , 15 + marginLeft ,line_y+Menu->font_size);
	
		if(Menu->shell[shellindex].next == 1)
		{	
			cairo_move_to(cr,Menu->width - marginRight - 6,line_y+4);
			cairo_line_to(cr,Menu->width - marginRight - 6,line_y+16);
			cairo_line_to(cr,Menu->width - marginRight - 1,line_y+10);
			cairo_line_to(cr,Menu->width - marginRight - 6,line_y+4);
			cairo_fill (cr);
		}
	}
	else
	{
		cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
		fcitx_cairo_set_color(cr, &sc.skinMenu.activeColor);
        cairo_rectangle (cr, marginLeft ,line_y, Menu->width - marginRight - marginLeft,Menu->font_size+4);
        cairo_fill (cr);
	
       
		fcitx_cairo_set_color(cr, &sc.skinFont.menuFontColor[MENU_ACTIVE]);
		cairo_select_font_face(cr, Menu->font,CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr, Menu->font_size);
	
        OutputStringWithContext(cr, Menu->shell[shellindex].tipstr , 15 + marginLeft ,line_y+Menu->font_size);
		
		if(Menu->shell[shellindex].next == 1)
		{
            cairo_move_to(cr,Menu->width - marginRight - 6,line_y+4);
			cairo_line_to(cr,Menu->width - marginRight - 6,line_y+16);
			cairo_line_to(cr,Menu->width - marginRight - 1,line_y+10);
			cairo_line_to(cr,Menu->width - marginRight - 6,line_y+4);
			cairo_fill (cr);
		}		
	}
    cairo_destroy(cr);
}

/**
*返回鼠标指向的菜单在menu中是第多少项
*/
int selectShellIndex(xlibMenu * Menu, int x, int y, int* offseth)
{
    int i;
    int winheight=sc.skinMenu.marginTop;
    int fontheight;
    int marginLeft = sc.skinMenu.marginLeft;

    if (x < marginLeft)
        return -1;
    
    fontheight=FontHeight(Menu->font);
    for(i=0;i<Menu->useItemCount;i++)
    {
        if( Menu->shell[i].type == menushell)
        {
            if(y>winheight+1 && y<winheight+6+fontheight-1)
            {
                if (offseth)
                    *offseth = winheight;
                return i;
            }
            winheight=winheight+6+fontheight;                
        }
        else if( Menu->shell[i].type == divline)
            winheight+=5;
    }
	return -1;
}

Bool colorRevese(xlibMenu * Menu,int shellIndex)
{
    Bool flag = False;
	int i;

    int last = -1;

	for(i=0;i<Menu->useItemCount;i++)
	{
        if (Menu->shell[i].isselect)
            last = i;

        Menu->shell[i].isselect=0;
	}
    if (shellIndex == last)
        flag = True;
    Menu->shell[shellIndex].isselect = 1;
    return flag;
}

void clearSelectFlag(xlibMenu * Menu)
{
	int i;
	for(i=0;i<16;i++)
	{
		Menu->shell[i].isselect=0;
	}
}


//以上为菜单的简单封装，下面为对菜单的操作部分
//=========================================================================================
//创建菜单窗口
Bool CreateMenuWindow( )
{
    load_menu_img();
    CreateMainMenuWindow();
	CreateImMenuWindow();   //创建输入法选择菜单窗口
	CreateSkinMenuWindow(); //创建皮肤选择菜单窗口
	CreateVKMenuWindow();	//创建软键盘布局选择菜单窗口
	return 0;
}

void DestroyMenuWindow()
{
    DestroyXlibMenu(&mainMenu);
    DestroyXlibMenu(&imMenu);
    DestroyXlibMenu(&skinMenu);
    DestroyXlibMenu(&vkMenu);
}

void DestroyXlibMenu(xlibMenu *menu)
{
    cairo_surface_destroy(menu->menu_cs);
    XDestroyWindow(dpy, menu->menuWindow);
}

Bool CreateMainMenuWindow() 
{
    int i;
	InitMenuDefault(&mainMenu);
	SetMeueShell(&mainMenu.shell[0],_("About Fcitx"),0,menushell);
	SetMeueShell(&mainMenu.shell[1],"",0,divline);
	SetMeueShell(&mainMenu.shell[2],_("Switch Skin"),0,menushell);
	SetMeueShell(&mainMenu.shell[3],_("Switch IM"),0,menushell);
	SetMeueShell(&mainMenu.shell[4],_("Switch VK"),0,menushell);
	SetMeueShell(&mainMenu.shell[5],"",0,divline);
	SetMeueShell(&mainMenu.shell[6],_("Fcitx Config ..."),0,menushell);
	SetMeueShell(&mainMenu.shell[7],_("Exit Fcitx"),0,menushell);
	mainMenu.useItemCount=8;
	for(i=0;i<mainMenu.useItemCount;i++)
	{
		mainMenu.shell[i].next=0;
	}
	mainMenu.shell[2].next=1;
	mainMenu.shell[3].next=1;
	mainMenu.shell[4].next=1;
	CreateXlibMenu(dpy,&mainMenu);
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
	InitMenuDefault(&vkMenu);
	SetMeueShell(&vkMenu.shell[0],"1.西文半角",0,menushell);
	SetMeueShell(&vkMenu.shell[1],"2.全角符号",0,menushell);
	SetMeueShell(&vkMenu.shell[2],"3.希腊字母",0,menushell);
	SetMeueShell(&vkMenu.shell[3],"4.俄文字母",0,menushell);
	SetMeueShell(&vkMenu.shell[4],"5.数字序号",0,menushell);
	SetMeueShell(&vkMenu.shell[5],"6.数学符号",0,menushell);
	SetMeueShell(&vkMenu.shell[6],"7.数字符号",0,menushell);
	SetMeueShell(&vkMenu.shell[7],"8.特殊符号",0,menushell);
	SetMeueShell(&vkMenu.shell[8],"9.日文平假名",0,menushell);
	SetMeueShell(&vkMenu.shell[9],"10.日文片假名",0,menushell);
	SetMeueShell(&vkMenu.shell[10],"11.制表符",0,menushell);
		
	vkMenu.useItemCount=11;
	ret=CreateXlibMenu(dpy,&vkMenu);
	return ret;
	
}

//主菜单事件处理
void MainMenuEvent(int x,int y)
{
	int i,j;
	char tmpstr[64]={0};
    int offseth = 0;
	i=selectShellIndex(&mainMenu, x, y, &offseth);
	Bool flag = colorRevese(&mainMenu,i);
    if (flag)
        return;
    
    DrawXlibMenu( dpy,&mainMenu);
    DisplayXlibMenu(dpy,&mainMenu);	
	
	switch(i)
	{
	//显示皮肤菜单
		case 2:
			skinMenu.useItemCount=skinBuf->i;
			for(j=0;j<skinBuf->i;j++)
			{
                char **sskin = (char**)utarray_eltptr(skinBuf, j);
				strcpy(skinMenu.shell[j].tipstr, *sskin);
			}
			skinMenu.pos_x=mainMenu.pos_x;
			skinMenu.pos_y=mainMenu.pos_y;
		
			if( skinMenu.pos_x+ mainMenu.width+skinMenu.width > DisplayWidth(dpy,iScreen))
				skinMenu.pos_x=skinMenu.pos_x - skinMenu.width + sc.skinMenu.marginLeft + 4;
			else
				skinMenu.pos_x=skinMenu.pos_x + mainMenu.width - sc.skinMenu.marginRight - 4;
			
			if( skinMenu.pos_y+offseth+skinMenu.height >DisplayHeight(dpy, iScreen))
				skinMenu.pos_y=DisplayHeight(dpy, iScreen)-skinMenu.height-10;
			else 
				skinMenu.pos_y=offseth+skinMenu.pos_y-sc.skinMenu.marginTop;
			
			clearSelectFlag(&skinMenu);
			DrawXlibMenu( dpy,&skinMenu);
			DisplayXlibMenu(dpy,&skinMenu);	
			break;
		case 3:
			for(i=0;i<iIMCount;i++)
			{
				memset(tmpstr,0,sizeof(tmpstr));
				sprintf(tmpstr,"%d.%s",i+1,_(im[i].strName));
				SetMeueShell(&imMenu.shell[i],tmpstr,0,menushell);
			}
			imMenu.useItemCount=iIMCount;
			
			imMenu.pos_x=mainMenu.pos_x;
			imMenu.pos_y=mainMenu.pos_y;
		
			if( imMenu.pos_x+ mainMenu.width+imMenu.width > DisplayWidth(dpy,iScreen))
				imMenu.pos_x=imMenu.pos_x-imMenu.width+ sc.skinMenu.marginLeft + 4;
			else
				imMenu.pos_x=imMenu.pos_x+ mainMenu.width - sc.skinMenu.marginRight-4;
			
			if( imMenu.pos_y+offseth+imMenu.height >DisplayHeight(dpy, iScreen))
				imMenu.pos_y=DisplayHeight(dpy, iScreen)-imMenu.height-10;
			else 
				imMenu.pos_y=offseth+imMenu.pos_y-sc.skinMenu.marginTop;
			
			clearSelectFlag(&imMenu);
			DrawXlibMenu( dpy,&imMenu);
			DisplayXlibMenu(dpy,&imMenu);	
			break;
		case 4:
			vkMenu.pos_x=mainMenu.pos_x;
			vkMenu.pos_y=mainMenu.pos_y;
		
			if( vkMenu.pos_x+ mainMenu.width+vkMenu.width > DisplayWidth(dpy,iScreen))
				vkMenu.pos_x=vkMenu.pos_x-vkMenu.width+ sc.skinMenu.marginLeft + 4;
			else
				vkMenu.pos_x=vkMenu.pos_x+ mainMenu.width - sc.skinMenu.marginRight-4;
			
			if( vkMenu.pos_y+offseth+vkMenu.height >DisplayHeight(dpy, iScreen))
				vkMenu.pos_y=DisplayHeight(dpy, iScreen)-vkMenu.height-10;
			else 
				vkMenu.pos_y=offseth+vkMenu.pos_y-sc.skinMenu.marginTop;
			
			clearSelectFlag(&vkMenu)	;
			DrawXlibMenu( dpy,&vkMenu);
			DisplayXlibMenu(dpy,&vkMenu);	
			break;
		default :
			break;
	}
	
	if(mainMenu.shell[2].isselect == 0)
		XUnmapWindow (dpy, skinMenu.menuWindow);
	if(mainMenu.shell[3].isselect == 0)
		XUnmapWindow (dpy, imMenu.menuWindow);
	if(mainMenu.shell[4].isselect == 0)
		XUnmapWindow (dpy, vkMenu.menuWindow);
}

//输入法菜单事件处理
void IMMenuEvent(int x,int y)
{
	int i;
	i=selectShellIndex(&imMenu,x, y, NULL);

    Bool flag = colorRevese(&imMenu,i);
    if (flag)
        return;
	DrawXlibMenu(dpy,&imMenu);
	DisplayXlibMenu(dpy,&imMenu);	
}

//虚拟键盘菜单事件处理
void VKMenuEvent(int x,int y)
{
	int i;
	i=selectShellIndex(&vkMenu,x, y, NULL);
	Bool flag = colorRevese(&vkMenu,i);

    if (flag)
        return;
	DrawXlibMenu(dpy,&vkMenu);
	DisplayXlibMenu(dpy,&vkMenu);	
}

//皮肤菜单事件处理
void SkinMenuEvent(int x,int y)
{
	int i;
	i=selectShellIndex(&skinMenu,x, y, NULL);
	Bool flag = colorRevese(&skinMenu,i);
    if (flag)
        return;
	DrawXlibMenu(dpy,&skinMenu);
	DisplayXlibMenu(dpy,&skinMenu);	
}

