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
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "fcitx/fcitx.h"

#include "skin.h"
#include "MenuWindow.h"
#include "fcitx-config/fcitx-config.h"
#include "fcitx/ui.h"


static void FilterPlacement(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg);

CONFIG_BINDING_BEGIN(FcitxSkin)
CONFIG_BINDING_REGISTER("SkinInfo", "Name", skinInfo.skinName)
CONFIG_BINDING_REGISTER("SkinInfo", "Version", skinInfo.skinVersion)
CONFIG_BINDING_REGISTER("SkinInfo", "Author", skinInfo.skinAuthor)
CONFIG_BINDING_REGISTER("SkinInfo", "Desc", skinInfo.skinDesc)

CONFIG_BINDING_REGISTER("SkinFont", "RespectDPI", skinFont.respectDPI)
CONFIG_BINDING_REGISTER("SkinFont", "FontSize", skinFont.fontSize)
CONFIG_BINDING_REGISTER("SkinFont", "MenuFontSize", skinFont.menuFontSize)
CONFIG_BINDING_REGISTER("SkinFont", "TipColor", skinFont.fontColor[MSG_TIPS])
CONFIG_BINDING_REGISTER("SkinFont", "InputColor", skinFont.fontColor[MSG_INPUT])
CONFIG_BINDING_REGISTER("SkinFont", "IndexColor", skinFont.fontColor[MSG_INDEX])
CONFIG_BINDING_REGISTER("SkinFont", "UserPhraseColor", skinFont.fontColor[MSG_USERPHR])
CONFIG_BINDING_REGISTER("SkinFont", "FirstCandColor", skinFont.fontColor[MSG_FIRSTCAND])
CONFIG_BINDING_REGISTER("SkinFont", "CodeColor", skinFont.fontColor[MSG_CODE])
CONFIG_BINDING_REGISTER("SkinFont", "OtherColor", skinFont.fontColor[MSG_OTHER])
CONFIG_BINDING_REGISTER("SkinFont", "ActiveMenuColor", skinFont.menuFontColor[MENU_ACTIVE])
CONFIG_BINDING_REGISTER("SkinFont", "InactiveMenuColor", skinFont.menuFontColor[MENU_INACTIVE])

CONFIG_BINDING_REGISTER("SkinMainBar", "BackImg", skinMainBar.backImg)
CONFIG_BINDING_REGISTER("SkinMainBar", "Logo", skinMainBar.logo)
CONFIG_BINDING_REGISTER("SkinMainBar", "Eng", skinMainBar.eng)
CONFIG_BINDING_REGISTER("SkinMainBar", "Active", skinMainBar.active)
CONFIG_BINDING_REGISTER("SkinMainBar", "MarginLeft", skinMainBar.marginLeft)
CONFIG_BINDING_REGISTER("SkinMainBar", "MarginRight", skinMainBar.marginRight)
CONFIG_BINDING_REGISTER("SkinMainBar", "MarginTop", skinMainBar.marginTop)
CONFIG_BINDING_REGISTER("SkinMainBar", "MarginBottom", skinMainBar.marginBottom)
CONFIG_BINDING_REGISTER("SkinMainBar", "FillVertical", skinMainBar.fillV)
CONFIG_BINDING_REGISTER("SkinMainBar", "FillHorizontal", skinMainBar.fillH)
CONFIG_BINDING_REGISTER_WITH_FILTER("SkinMainBar", "Placement", skinMainBar.placement, FilterPlacement)
CONFIG_BINDING_REGISTER("SkinMainBar", "UseCustomTextIconColor", skinMainBar.bUseCustomTextIconColor)
CONFIG_BINDING_REGISTER("SkinMainBar", "ActiveTextIconColor", skinMainBar.textIconColor[0])
CONFIG_BINDING_REGISTER("SkinMainBar", "InactiveTextIconColor", skinMainBar.textIconColor[1])

CONFIG_BINDING_REGISTER("SkinInputBar", "BackImg", skinInputBar.backImg)
CONFIG_BINDING_REGISTER("SkinInputBar", "MarginTop", skinInputBar.marginTop)
CONFIG_BINDING_REGISTER("SkinInputBar", "MarginBottom", skinInputBar.marginBottom)
CONFIG_BINDING_REGISTER("SkinInputBar", "MarginLeft", skinInputBar.marginLeft)
CONFIG_BINDING_REGISTER("SkinInputBar", "MarginRight", skinInputBar.marginRight)
CONFIG_BINDING_REGISTER("SkinInputBar", "CursorColor", skinInputBar.cursorColor)
CONFIG_BINDING_REGISTER("SkinInputBar", "InputPos", skinInputBar.iInputPos)
CONFIG_BINDING_REGISTER("SkinInputBar", "OutputPos", skinInputBar.iOutputPos)
CONFIG_BINDING_REGISTER("SkinInputBar", "BackArrow", skinInputBar.backArrow)
CONFIG_BINDING_REGISTER("SkinInputBar", "ForwardArrow", skinInputBar.forwardArrow)
CONFIG_BINDING_REGISTER("SkinInputBar", "BackArrowX", skinInputBar.iBackArrowX)
CONFIG_BINDING_REGISTER("SkinInputBar", "BackArrowY", skinInputBar.iBackArrowY)
CONFIG_BINDING_REGISTER("SkinInputBar", "ForwardArrowX", skinInputBar.iForwardArrowX)
CONFIG_BINDING_REGISTER("SkinInputBar", "ForwardArrowY", skinInputBar.iForwardArrowY)
CONFIG_BINDING_REGISTER("SkinInputBar", "FillVertical", skinInputBar.fillV)
CONFIG_BINDING_REGISTER("SkinInputBar", "FillHorizontal", skinInputBar.fillH)

CONFIG_BINDING_REGISTER("SkinTrayIcon", "Active", skinTrayIcon.active)
CONFIG_BINDING_REGISTER("SkinTrayIcon", "Inactive", skinTrayIcon.inactive)

CONFIG_BINDING_REGISTER("SkinMenu", "BackImg", skinMenu.backImg)
CONFIG_BINDING_REGISTER("SkinMenu", "MarginTop", skinMenu.marginTop)
CONFIG_BINDING_REGISTER("SkinMenu", "MarginBottom", skinMenu.marginBottom)
CONFIG_BINDING_REGISTER("SkinMenu", "MarginLeft", skinMenu.marginLeft)
CONFIG_BINDING_REGISTER("SkinMenu", "MarginRight", skinMenu.marginRight)
CONFIG_BINDING_REGISTER("SkinMenu", "ActiveColor", skinMenu.activeColor)
CONFIG_BINDING_REGISTER("SkinMenu", "LineColor", skinMenu.lineColor)
CONFIG_BINDING_REGISTER("SkinMenu", "FillVertical", skinMenu.fillV)
CONFIG_BINDING_REGISTER("SkinMenu", "FillHorizontal", skinMenu.fillH)

CONFIG_BINDING_REGISTER("SkinKeyboard", "BackImg", skinKeyboard.backImg)
CONFIG_BINDING_REGISTER("SkinKeyboard", "KeyColor", skinKeyboard.keyColor)

CONFIG_BINDING_END()

void FilterPlacement(FcitxGenericConfig* config, FcitxConfigGroup *group, FcitxConfigOption *option, void* value, FcitxConfigSync sync, void* arg)
{
    FcitxSkin* sc = (FcitxSkin*) config;
    if (sync == Raw2Value) {
        ParsePlacement(&sc->skinMainBar.skinPlacement, *(char**) value);
    }
}
// kate: indent-mode cstyle; space-indent on; indent-width 0;
