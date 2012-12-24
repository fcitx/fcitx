/***************************************************************************
 *   Copyright (C) 2010~2010 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
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
 * @file hook-internal.h
 * private header of hook
 */

struct _FcitxInstance;

/**
 * do the preinput phase
 *
 * @param instance fcitx instance
 * @param sym keysym
 * @param state keystate
 * @param retval input return val
 * @return void
 **/
void FcitxInstanceProcessPreInputFilter(struct _FcitxInstance* instance, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval);

/**
 * do the postinput phase
 *
 * @param instance fcitx instance
 * @param sym keysym
 * @param state keystate
 * @param retval input return val
 * @return void
 **/
void FcitxInstanceProcessPostInputFilter(struct _FcitxInstance* instance, FcitxKeySym sym, unsigned int state, INPUT_RETURN_VALUE* retval);

/**
 * process hotkey phase
 *
 * @param instance fcitx instance
 * @param sym keysym
 * @param state keystate
 * @return INPUT_RETURN_VALUE
 **/
INPUT_RETURN_VALUE FcitxInstanceProcessHotkey(struct _FcitxInstance* instance, FcitxKeySym keysym, unsigned int state);

/**
 * process reset input
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessResetInputHook(struct _FcitxInstance* instance);

/**
 * process trigger off event
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessTriggerOffHook(struct _FcitxInstance* instance);

/**
 * process trigger on event
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessTriggerOnHook(struct _FcitxInstance* instance);

/**
 * process focus in event
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessInputFocusHook(struct _FcitxInstance* instance);
/**
 * process focus out event
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessInputUnFocusHook(struct _FcitxInstance* instance);

/**
 * process update candidates event
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessUpdateCandidates(struct _FcitxInstance* instance);

/**
 * process update im list
 *
 * @param instance fcitx instance
 * @return void
 **/
void FcitxInstanceProcessUpdateIMListHook(struct _FcitxInstance* instance);

void FcitxInstanceProcessICStateChangedHook(struct _FcitxInstance* instance, struct _FcitxInputContext* ic);

void FcitxInstanceProcessIMChangedHook(struct _FcitxInstance* instance);

// kate: indent-mode cstyle; space-indent on; indent-width 0;
