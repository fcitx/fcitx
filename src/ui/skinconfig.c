/***************************************************************************
 *   Copyright (C) 2010~2010 by t3swing                                    *
 *   t3swing@gmail.com                                                     *
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

#include "ui/skin.h"
#include "ui/MenuWindow.h"
#include "fcitx-config/fcitx-config.h"

CONFIG_BINDING_BEGIN(FcitxSkin);
CONFIG_BINDING_REGISTER("SkinInfo","Name",skinInfo.skinName);
CONFIG_BINDING_REGISTER("SkinInfo","Version",skinInfo.skinVersion);
CONFIG_BINDING_REGISTER("SkinInfo","Author",skinInfo.skinAuthor);
CONFIG_BINDING_REGISTER("SkinInfo","Desc",skinInfo.skinDesc);
	
CONFIG_BINDING_REGISTER("SkinFont","FontSize",skinFont.fontSize);
CONFIG_BINDING_REGISTER("SkinFont","TipColor",skinFont.fontColor[MSG_TIPS]);
CONFIG_BINDING_REGISTER("SkinFont","InputColor",skinFont.fontColor[MSG_INPUT]);
CONFIG_BINDING_REGISTER("SkinFont","IndexColor",skinFont.fontColor[MSG_INDEX]);
CONFIG_BINDING_REGISTER("SkinFont","UserPhraseColor",skinFont.fontColor[MSG_USERPHR]);
CONFIG_BINDING_REGISTER("SkinFont","FirstCandColor",skinFont.fontColor[MSG_FIRSTCAND]);
CONFIG_BINDING_REGISTER("SkinFont","CodeColor",skinFont.fontColor[MSG_CODE]);
CONFIG_BINDING_REGISTER("SkinFont","OtherColor",skinFont.fontColor[MSG_OTHER]);
CONFIG_BINDING_REGISTER("SkinFont","ActiveMenuColor",skinFont.menuFontColor[MENU_ACTIVE]);
CONFIG_BINDING_REGISTER("SkinFont","InactiveMenuColor",skinFont.menuFontColor[MENU_INACTIVE]);
	
CONFIG_BINDING_REGISTER("SkinMainBar","BackImg",skinMainBar.backImg);
CONFIG_BINDING_REGISTER("SkinMainBar","Logo",skinMainBar.logo);
CONFIG_BINDING_REGISTER("SkinMainBar","ZhPunc",skinMainBar.zhpunc);
CONFIG_BINDING_REGISTER("SkinMainBar","EnPunc",skinMainBar.enpunc);
CONFIG_BINDING_REGISTER("SkinMainBar","Chs",skinMainBar.chs);
CONFIG_BINDING_REGISTER("SkinMainBar","Cht",skinMainBar.cht);
CONFIG_BINDING_REGISTER("SkinMainBar","HalfCorner",skinMainBar.halfcorner);
CONFIG_BINDING_REGISTER("SkinMainBar","FullCorner",skinMainBar.fullcorner);
CONFIG_BINDING_REGISTER("SkinMainBar","Unlock",skinMainBar.unlock);
CONFIG_BINDING_REGISTER("SkinMainBar","Lock",  skinMainBar.lock);
CONFIG_BINDING_REGISTER("SkinMainBar","Legend",skinMainBar.legend);
CONFIG_BINDING_REGISTER("SkinMainBar","NoLegend",skinMainBar.nolegend);
CONFIG_BINDING_REGISTER("SkinMainBar","VK",skinMainBar.vk);
CONFIG_BINDING_REGISTER("SkinMainBar","NoVK",skinMainBar.novk);
CONFIG_BINDING_REGISTER("SkinMainBar","Eng",skinMainBar.eng);
CONFIG_BINDING_REGISTER("SkinMainBar","Chn",skinMainBar.chn);
CONFIG_BINDING_REGISTER("SkinInputBar","BackImg",skinInputBar.backImg);
CONFIG_BINDING_REGISTER("SkinInputBar","Resize", skinInputBar.resize);
CONFIG_BINDING_REGISTER("SkinInputBar","ResizePos", skinInputBar.resizePos);
CONFIG_BINDING_REGISTER("SkinInputBar","ResizeWidth", skinInputBar.resizeWidth);
CONFIG_BINDING_REGISTER("SkinInputBar","InputPos", skinInputBar.inputPos);
CONFIG_BINDING_REGISTER("SkinInputBar","OutputPos",skinInputBar.outputPos);
CONFIG_BINDING_REGISTER("SkinInputBar","LayoutLeft", skinInputBar.layoutLeft);
CONFIG_BINDING_REGISTER("SkinInputBar","LayoutRight", skinInputBar.layoutRight);
CONFIG_BINDING_REGISTER("SkinInputBar","CursorColor",skinInputBar.cursorColor);
CONFIG_BINDING_REGISTER("SkinInputBar","BackArrow",skinInputBar.backArrow);
CONFIG_BINDING_REGISTER("SkinInputBar","ForwardArrow",skinInputBar.forwardArrow);

CONFIG_BINDING_REGISTER("SkinTrayIcon","Active",skinTrayIcon.active);
CONFIG_BINDING_REGISTER("SkinTrayIcon","Inactive",skinTrayIcon.inactive);

CONFIG_BINDING_REGISTER("SkinMenu", "BackImg", skinMenu.backImg);
CONFIG_BINDING_REGISTER("SkinMenu", "Resize", skinMenu.resize);
CONFIG_BINDING_REGISTER("SkinMenu", "MarginTop", skinMenu.marginTop);
CONFIG_BINDING_REGISTER("SkinMenu", "MarginBottom", skinMenu.marginBottom);
CONFIG_BINDING_REGISTER("SkinMenu", "MarginLeft", skinMenu.marginLeft);
CONFIG_BINDING_REGISTER("SkinMenu", "MarginRight", skinMenu.marginRight);
CONFIG_BINDING_REGISTER("SkinMenu", "ActiveColor", skinMenu.activeColor);
CONFIG_BINDING_REGISTER("SkinMenu", "LineColor", skinMenu.lineColor);

CONFIG_BINDING_END()

