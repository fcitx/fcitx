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

#include <stdlib.h>
#include <string.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/utils.h"
#include "hotkey.h"
#include "keydata.h"

/**
 * String to key list.
 **/

typedef struct _KEY_LIST {
    /**
     * string name for the key in fcitx
     **/
    char         *strKey;
    /**
     * the keyval for the key.
     **/
    FcitxKeySym  code;
} KEY_LIST;

/* fcitx key name translist */
KEY_LIST        keyList[] = {
    {"TAB", FcitxKey_Tab},
    {"ENTER", FcitxKey_Return},
    {"LCTRL", FcitxKey_Control_L},
    {"LSHIFT", FcitxKey_Shift_L},
    {"LALT", FcitxKey_Alt_L},
    {"RCTRL", FcitxKey_Control_R},
    {"RSHIFT", FcitxKey_Shift_R},
    {"RALT", FcitxKey_Alt_R},
    {"INSERT", FcitxKey_Insert},
    {"HOME", FcitxKey_Home},
    {"PGUP", FcitxKey_Page_Up},
    {"END", FcitxKey_End},
    {"PGDN", FcitxKey_Page_Down},
    {"ESCAPE", FcitxKey_Escape},
    {"SPACE", FcitxKey_space},
    {"BACKSPACE", FcitxKey_BackSpace},
    {"DELETE", FcitxKey_Delete},
    {"UP", FcitxKey_Up},
    {"DOWN", FcitxKey_Down},
    {"LEFT", FcitxKey_Left},
    {"RIGHT", FcitxKey_Right},
    {"LINEFEED", FcitxKey_Linefeed },
    {"CLEAR", FcitxKey_Clear },
    {"MULTIKEY", FcitxKey_Multi_key },
    {"CODEINPUT", FcitxKey_Codeinput },
    {"SINGLECANDIDATE", FcitxKey_SingleCandidate },
    {"MULTIPLECANDIDATE", FcitxKey_MultipleCandidate },
    {"PREVIOUSCANDIDATE", FcitxKey_PreviousCandidate },
    {"KANJI", FcitxKey_Kanji },
    {"MUHENKAN", FcitxKey_Muhenkan },
    {"HENKANMODE", FcitxKey_Henkan_Mode },
    {"HENKAN", FcitxKey_Henkan },
    {"ROMAJI", FcitxKey_Romaji },
    {"HIRAGANA", FcitxKey_Hiragana },
    {"KATAKANA", FcitxKey_Katakana },
    {"HIRAGANAKATAKANA", FcitxKey_Hiragana_Katakana },
    {"ZENKAKU", FcitxKey_Zenkaku },
    {"HANKAKU", FcitxKey_Hankaku },
    {"ZENKAKUHANKAKU", FcitxKey_Zenkaku_Hankaku },
    {"TOUROKU", FcitxKey_Touroku },
    {"MASSYO", FcitxKey_Massyo },
    {"KANALOCK", FcitxKey_Kana_Lock },
    {"KANASHIFT", FcitxKey_Kana_Shift },
    {"EISUSHIFT", FcitxKey_Eisu_Shift },
    {"EISUTOGGLE", FcitxKey_Eisu_toggle },
    {"KANJIBANGOU", FcitxKey_Kanji_Bangou },
    {"ZENKOHO", FcitxKey_Zen_Koho },
    {"MAEKOHO", FcitxKey_Mae_Koho },
    {"F1", FcitxKey_F1 },
    {"F2", FcitxKey_F2 },
    {"F3", FcitxKey_F3 },
    {"F4", FcitxKey_F4 },
    {"F5", FcitxKey_F5 },
    {"F6", FcitxKey_F6 },
    {"F7", FcitxKey_F7 },
    {"F8", FcitxKey_F8 },
    {"F9", FcitxKey_F9 },
    {"F10", FcitxKey_F10 },
    {"F11", FcitxKey_F11 },
    {"L1", FcitxKey_L1 },
    {"F12", FcitxKey_F12 },
    {"L2", FcitxKey_L2 },
    {"F13", FcitxKey_F13 },
    {"L3", FcitxKey_L3 },
    {"F14", FcitxKey_F14 },
    {"L4", FcitxKey_L4 },
    {"F15", FcitxKey_F15 },
    {"L5", FcitxKey_L5 },
    {"F16", FcitxKey_F16 },
    {"L6", FcitxKey_L6 },
    {"F17", FcitxKey_F17 },
    {"L7", FcitxKey_L7 },
    {"F18", FcitxKey_F18 },
    {"L8", FcitxKey_L8 },
    {"F19", FcitxKey_F19 },
    {"L9", FcitxKey_L9 },
    {"F20", FcitxKey_F20 },
    {"L10", FcitxKey_L10 },
    {"F21", FcitxKey_F21 },
    {"R1", FcitxKey_R1 },
    {"F22", FcitxKey_F22 },
    {"R2", FcitxKey_R2 },
    {"F23", FcitxKey_F23 },
    {"R3", FcitxKey_R3 },
    {"F24", FcitxKey_F24 },
    {"R4", FcitxKey_R4 },
    {"F25", FcitxKey_F25 },
    {"R5", FcitxKey_R5 },
    {"F26", FcitxKey_F26 },
    {"R6", FcitxKey_R6 },
    {"F27", FcitxKey_F27 },
    {"R7", FcitxKey_R7 },
    {"F28", FcitxKey_F28 },
    {"R8", FcitxKey_R8 },
    {"F29", FcitxKey_F29 },
    {"R9", FcitxKey_R9 },
    {"F30", FcitxKey_F30 },
    {"R10", FcitxKey_R10 },
    {"F31", FcitxKey_F31 },
    {"R11", FcitxKey_R11 },
    {"F32", FcitxKey_F32 },
    {"R12", FcitxKey_R12 },
    {"F33", FcitxKey_F33 },
    {"R13", FcitxKey_R13 },
    {"F34", FcitxKey_F34 },
    {"R14", FcitxKey_R14 },
    {"F35", FcitxKey_F35 },
    {"R15", FcitxKey_R15 },
    {"FIRSTVIRTUALSCREEN", FcitxKey_First_Virtual_Screen },
    {"PREVVIRTUALSCREEN", FcitxKey_Prev_Virtual_Screen },
    {"NEXTVIRTUALSCREEN", FcitxKey_Next_Virtual_Screen },
    {"LASTVIRTUALSCREEN", FcitxKey_Last_Virtual_Screen },
    {"TERMINATESERVER", FcitxKey_Terminate_Server },
    {"ACCESSXENABLE", FcitxKey_AccessX_Enable },
    {"ACCESSXFEEDBACKENABLE", FcitxKey_AccessX_Feedback_Enable },
    {"REPEATKEYSENABLE", FcitxKey_RepeatKeys_Enable },
    {"SLOWKEYSENABLE", FcitxKey_SlowKeys_Enable },
    {"BOUNCEKEYSENABLE", FcitxKey_BounceKeys_Enable },
    {"STICKYKEYSENABLE", FcitxKey_StickyKeys_Enable },
    {"MOUSEKEYSENABLE", FcitxKey_MouseKeys_Enable },
    {"MOUSEKEYSACCELENABLE", FcitxKey_MouseKeys_Accel_Enable },
    {"OVERLAY1ENABLE", FcitxKey_Overlay1_Enable },
    {"OVERLAY2ENABLE", FcitxKey_Overlay2_Enable },
    {"AUDIBLEBELLENABLE", FcitxKey_AudibleBell_Enable },
    {"KAPPA", FcitxKey_kappa },
    {"KANAMIDDLEDOT", FcitxKey_kana_middledot },
    {"KANATU", FcitxKey_kana_tu },
    {"KANATI", FcitxKey_kana_TI },
    {"KANATU", FcitxKey_kana_TU },
    {"KANAHU", FcitxKey_kana_HU },
    {"KANASWITCH", FcitxKey_kana_switch },
    {"ARABICHEH", FcitxKey_Arabic_heh },
    {"ARABICSWITCH", FcitxKey_Arabic_switch },
    {"UKRANIANJE", FcitxKey_Ukranian_je },
    {"UKRANIANI", FcitxKey_Ukranian_i },
    {"UKRANIANYI", FcitxKey_Ukranian_yi },
    {"SERBIANJE", FcitxKey_Serbian_je },
    {"SERBIANLJE", FcitxKey_Serbian_lje },
    {"SERBIANNJE", FcitxKey_Serbian_nje },
    {"SERBIANDZE", FcitxKey_Serbian_dze },
    {"UKRANIANJE", FcitxKey_Ukranian_JE },
    {"UKRANIANI", FcitxKey_Ukranian_I },
    {"UKRANIANYI", FcitxKey_Ukranian_YI },
    {"SERBIANJE", FcitxKey_Serbian_JE },
    {"SERBIANLJE", FcitxKey_Serbian_LJE },
    {"SERBIANNJE", FcitxKey_Serbian_NJE },
    {"SERBIANDZE", FcitxKey_Serbian_DZE },
    {"GREEKIOTADIAERESIS", FcitxKey_Greek_IOTAdiaeresis },
    {"GREEKSWITCH", FcitxKey_Greek_switch },
    {"TOPLEFTSUMMATION", FcitxKey_topleftsummation },
    {"BOTLEFTSUMMATION", FcitxKey_botleftsummation },
    {"TOPVERTSUMMATIONCONNECTOR", FcitxKey_topvertsummationconnector },
    {"BOTVERTSUMMATIONCONNECTOR", FcitxKey_botvertsummationconnector },
    {"TOPRIGHTSUMMATION", FcitxKey_toprightsummation },
    {"BOTRIGHTSUMMATION", FcitxKey_botrightsummation },
    {"RIGHTMIDDLESUMMATION", FcitxKey_rightmiddlesummation },
    {"BLANK", FcitxKey_blank },
    {"MARKER", FcitxKey_marker },
    {"TRADEMARKINCIRCLE", FcitxKey_trademarkincircle },
    {"HEXAGRAM", FcitxKey_hexagram },
    {"CURSOR", FcitxKey_cursor },
    {"HEBREWBETH", FcitxKey_hebrew_beth },
    {"HEBREWGIMMEL", FcitxKey_hebrew_gimmel },
    {"HEBREWDALETH", FcitxKey_hebrew_daleth },
    {"HEBREWZAYIN", FcitxKey_hebrew_zayin },
    {"HEBREWHET", FcitxKey_hebrew_het },
    {"HEBREWTETH", FcitxKey_hebrew_teth },
    {"HEBREWSAMEKH", FcitxKey_hebrew_samekh },
    {"HEBREWFINALZADI", FcitxKey_hebrew_finalzadi },
    {"HEBREWZADI", FcitxKey_hebrew_zadi },
    {"HEBREWKUF", FcitxKey_hebrew_kuf },
    {"HEBREWTAF", FcitxKey_hebrew_taf },
    {"HEBREWSWITCH", FcitxKey_Hebrew_switch },
    {"THAIMAIHANAKATMAITHO", FcitxKey_Thai_maihanakat_maitho },
    {"HANGUL", FcitxKey_Hangul },
    {"HANGULSTART", FcitxKey_Hangul_Start },
    {"HANGULEND", FcitxKey_Hangul_End },
    {"HANGULHANJA", FcitxKey_Hangul_Hanja },
    {"HANGULJAMO", FcitxKey_Hangul_Jamo },
    {"HANGULROMAJA", FcitxKey_Hangul_Romaja },
    {"HANGULCODEINPUT", FcitxKey_Hangul_Codeinput },
    {"HANGULJEONJA", FcitxKey_Hangul_Jeonja },
    {"HANGULBANJA", FcitxKey_Hangul_Banja },
    {"HANGULPREHANJA", FcitxKey_Hangul_PreHanja },
    {"HANGULPOSTHANJA", FcitxKey_Hangul_PostHanja },
    {"HANGULSINGLECANDIDATE", FcitxKey_Hangul_SingleCandidate },
    {"HANGULMULTIPLECANDIDATE", FcitxKey_Hangul_MultipleCandidate },
    {"HANGULPREVIOUSCANDIDATE", FcitxKey_Hangul_PreviousCandidate },
    {"HANGULSPECIAL", FcitxKey_Hangul_Special },
    {"HANGULSWITCH", FcitxKey_Hangul_switch },
    {"HANGULKIYEOG", FcitxKey_Hangul_Kiyeog },
    {"HANGULSSANGKIYEOG", FcitxKey_Hangul_SsangKiyeog },
    {"HANGULKIYEOGSIOS", FcitxKey_Hangul_KiyeogSios },
    {"HANGULNIEUN", FcitxKey_Hangul_Nieun },
    {"HANGULNIEUNJIEUJ", FcitxKey_Hangul_NieunJieuj },
    {"HANGULNIEUNHIEUH", FcitxKey_Hangul_NieunHieuh },
    {"HANGULDIKEUD", FcitxKey_Hangul_Dikeud },
    {"HANGULSSANGDIKEUD", FcitxKey_Hangul_SsangDikeud },
    {"HANGULRIEUL", FcitxKey_Hangul_Rieul },
    {"HANGULRIEULKIYEOG", FcitxKey_Hangul_RieulKiyeog },
    {"HANGULRIEULMIEUM", FcitxKey_Hangul_RieulMieum },
    {"HANGULRIEULPIEUB", FcitxKey_Hangul_RieulPieub },
    {"HANGULRIEULSIOS", FcitxKey_Hangul_RieulSios },
    {"HANGULRIEULTIEUT", FcitxKey_Hangul_RieulTieut },
    {"HANGULRIEULPHIEUF", FcitxKey_Hangul_RieulPhieuf },
    {"HANGULRIEULHIEUH", FcitxKey_Hangul_RieulHieuh },
    {"HANGULMIEUM", FcitxKey_Hangul_Mieum },
    {"HANGULPIEUB", FcitxKey_Hangul_Pieub },
    {"HANGULSSANGPIEUB", FcitxKey_Hangul_SsangPieub },
    {"HANGULPIEUBSIOS", FcitxKey_Hangul_PieubSios },
    {"HANGULSIOS", FcitxKey_Hangul_Sios },
    {"HANGULSSANGSIOS", FcitxKey_Hangul_SsangSios },
    {"HANGULIEUNG", FcitxKey_Hangul_Ieung },
    {"HANGULJIEUJ", FcitxKey_Hangul_Jieuj },
    {"HANGULSSANGJIEUJ", FcitxKey_Hangul_SsangJieuj },
    {"HANGULCIEUC", FcitxKey_Hangul_Cieuc },
    {"HANGULKHIEUQ", FcitxKey_Hangul_Khieuq },
    {"HANGULTIEUT", FcitxKey_Hangul_Tieut },
    {"HANGULPHIEUF", FcitxKey_Hangul_Phieuf },
    {"HANGULHIEUH", FcitxKey_Hangul_Hieuh },
    {"HANGULA", FcitxKey_Hangul_A },
    {"HANGULAE", FcitxKey_Hangul_AE },
    {"HANGULYA", FcitxKey_Hangul_YA },
    {"HANGULYAE", FcitxKey_Hangul_YAE },
    {"HANGULEO", FcitxKey_Hangul_EO },
    {"HANGULE", FcitxKey_Hangul_E },
    {"HANGULYEO", FcitxKey_Hangul_YEO },
    {"HANGULYE", FcitxKey_Hangul_YE },
    {"HANGULO", FcitxKey_Hangul_O },
    {"HANGULWA", FcitxKey_Hangul_WA },
    {"HANGULWAE", FcitxKey_Hangul_WAE },
    {"HANGULOE", FcitxKey_Hangul_OE },
    {"HANGULYO", FcitxKey_Hangul_YO },
    {"HANGULU", FcitxKey_Hangul_U },
    {"HANGULWEO", FcitxKey_Hangul_WEO },
    {"HANGULWE", FcitxKey_Hangul_WE },
    {"HANGULWI", FcitxKey_Hangul_WI },
    {"HANGULYU", FcitxKey_Hangul_YU },
    {"HANGULEU", FcitxKey_Hangul_EU },
    {"HANGULYI", FcitxKey_Hangul_YI },
    {"HANGULI", FcitxKey_Hangul_I },
    {"HANGULJKIYEOG", FcitxKey_Hangul_J_Kiyeog },
    {"HANGULJSSANGKIYEOG", FcitxKey_Hangul_J_SsangKiyeog },
    {"HANGULJKIYEOGSIOS", FcitxKey_Hangul_J_KiyeogSios },
    {"HANGULJNIEUN", FcitxKey_Hangul_J_Nieun },
    {"HANGULJNIEUNJIEUJ", FcitxKey_Hangul_J_NieunJieuj },
    {"HANGULJNIEUNHIEUH", FcitxKey_Hangul_J_NieunHieuh },
    {"HANGULJDIKEUD", FcitxKey_Hangul_J_Dikeud },
    {"HANGULJRIEUL", FcitxKey_Hangul_J_Rieul },
    {"HANGULJRIEULKIYEOG", FcitxKey_Hangul_J_RieulKiyeog },
    {"HANGULJRIEULMIEUM", FcitxKey_Hangul_J_RieulMieum },
    {"HANGULJRIEULPIEUB", FcitxKey_Hangul_J_RieulPieub },
    {"HANGULJRIEULSIOS", FcitxKey_Hangul_J_RieulSios },
    {"HANGULJRIEULTIEUT", FcitxKey_Hangul_J_RieulTieut },
    {"HANGULJRIEULPHIEUF", FcitxKey_Hangul_J_RieulPhieuf },
    {"HANGULJRIEULHIEUH", FcitxKey_Hangul_J_RieulHieuh },
    {"HANGULJMIEUM", FcitxKey_Hangul_J_Mieum },
    {"HANGULJPIEUB", FcitxKey_Hangul_J_Pieub },
    {"HANGULJPIEUBSIOS", FcitxKey_Hangul_J_PieubSios },
    {"HANGULJSIOS", FcitxKey_Hangul_J_Sios },
    {"HANGULJSSANGSIOS", FcitxKey_Hangul_J_SsangSios },
    {"HANGULJIEUNG", FcitxKey_Hangul_J_Ieung },
    {"HANGULJJIEUJ", FcitxKey_Hangul_J_Jieuj },
    {"HANGULJCIEUC", FcitxKey_Hangul_J_Cieuc },
    {"HANGULJKHIEUQ", FcitxKey_Hangul_J_Khieuq },
    {"HANGULJTIEUT", FcitxKey_Hangul_J_Tieut },
    {"HANGULJPHIEUF", FcitxKey_Hangul_J_Phieuf },
    {"HANGULJHIEUH", FcitxKey_Hangul_J_Hieuh },
    {"HANGULRIEULYEORINHIEUH", FcitxKey_Hangul_RieulYeorinHieuh },
    {"HANGULSUNKYEONGEUMMIEUM", FcitxKey_Hangul_SunkyeongeumMieum },
    {"HANGULSUNKYEONGEUMPIEUB", FcitxKey_Hangul_SunkyeongeumPieub },
    {"HANGULPANSIOS", FcitxKey_Hangul_PanSios },
    {"HANGULKKOGJIDALRINIEUNG", FcitxKey_Hangul_KkogjiDalrinIeung },
    {"HANGULSUNKYEONGEUMPHIEUF", FcitxKey_Hangul_SunkyeongeumPhieuf },
    {"HANGULYEORINHIEUH", FcitxKey_Hangul_YeorinHieuh },
    {"HANGULARAEA", FcitxKey_Hangul_AraeA },
    {"HANGULARAEAE", FcitxKey_Hangul_AraeAE },
    {"HANGULJPANSIOS", FcitxKey_Hangul_J_PanSios },
    {"HANGULJKKOGJIDALRINIEUNG", FcitxKey_Hangul_J_KkogjiDalrinIeung },
    {"HANGULJYEORINHIEUH", FcitxKey_Hangul_J_YeorinHieuh },
    {"BRAILLEDOT1", FcitxKey_braille_dot_1 },
    {"BRAILLEDOT2", FcitxKey_braille_dot_2 },
    {"BRAILLEDOT3", FcitxKey_braille_dot_3 },
    {"BRAILLEDOT4", FcitxKey_braille_dot_4 },
    {"BRAILLEDOT5", FcitxKey_braille_dot_5 },
    {"BRAILLEDOT6", FcitxKey_braille_dot_6 },
    {"BRAILLEDOT7", FcitxKey_braille_dot_7 },
    {"BRAILLEDOT8", FcitxKey_braille_dot_8 },
    {"BRAILLEDOT9", FcitxKey_braille_dot_9 },
    {"BRAILLEDOT10",FcitxKey_braille_dot_10 },
    {"SELECT",      FcitxKey_Select },
    {"EXECUTE",     FcitxKey_Execute },
    {"PRINT",       FcitxKey_Print },
    {"UNDO",        FcitxKey_Undo },
    {"REDO",        FcitxKey_Redo },
    {"MENU",        FcitxKey_Menu },
    {"FIND",        FcitxKey_Find },
    {"CANCEL",      FcitxKey_Cancel },
    {"HELP",        FcitxKey_Help },
    {"BREAK",       FcitxKey_Break },
    {"PAUSE", FcitxKey_Pause},
    {"SCROLLLOCK", FcitxKey_Scroll_Lock},
    {"SYSREQ", FcitxKey_Sys_Req},
    {"BEGIN", FcitxKey_Begin},
    {"MODESWITCH", FcitxKey_Mode_switch},
    {"SCRIPTSWITCH", FcitxKey_script_switch},
    {"NUMLOCK", FcitxKey_Num_Lock},
    {"KPSPACE", FcitxKey_KP_Space},
    {"KPTAB", FcitxKey_KP_Tab},
    {"KPENTER", FcitxKey_KP_Enter},
    {"KPF1", FcitxKey_KP_F1},
    {"KPF2", FcitxKey_KP_F2},
    {"KPF3", FcitxKey_KP_F3},
    {"KPF4", FcitxKey_KP_F4},
    {"KPHOME", FcitxKey_KP_Home},
    {"KPLEFT", FcitxKey_KP_Left},
    {"KPUP", FcitxKey_KP_Up},
    {"KPRIGHT", FcitxKey_KP_Right},
    {"KPDOWN", FcitxKey_KP_Down},
    {"KPPGUP", FcitxKey_KP_Page_Up},
    {"KPPGDOWN", FcitxKey_KP_Page_Down},
    {"KPEND", FcitxKey_KP_End},
    {"KPBEGIN", FcitxKey_KP_Begin},
    {"KPINSERT", FcitxKey_KP_Insert},
    {"KPDELETE", FcitxKey_KP_Delete},
    {"KPEQUAL", FcitxKey_KP_Equal},
    {"KPMULTIPLY", FcitxKey_KP_Multiply},
    {"KPADD", FcitxKey_KP_Add},
    {"KPSEPARATOR", FcitxKey_KP_Separator},
    {"KPSUBTRACT", FcitxKey_KP_Subtract},
    {"KPDECIMAL", FcitxKey_KP_Decimal},
    {"KPDIVIDE", FcitxKey_KP_Divide},
    {"KP0", FcitxKey_KP_0},
    {"KP1", FcitxKey_KP_1},
    {"KP2", FcitxKey_KP_2},
    {"KP3", FcitxKey_KP_3},
    {"KP4", FcitxKey_KP_4},
    {"KP5", FcitxKey_KP_5},
    {"KP6", FcitxKey_KP_6},
    {"KP7", FcitxKey_KP_7},
    {"KP8", FcitxKey_KP_8},
    {"KP9", FcitxKey_KP_9},
    {"CAPSLOCK", FcitxKey_Caps_Lock},
    {"SHIFTLOCK", FcitxKey_Shift_Lock},
    {"LMETA", FcitxKey_Meta_L},
    {"RMETA", FcitxKey_Meta_R},
    {"LSUPER", FcitxKey_Super_L},
    {"RSUPER", FcitxKey_Super_R},
    {"LHYPER", FcitxKey_Hyper_L},
    {"RHYPER", FcitxKey_Hyper_R},


    {"\0", 0}
};


FCITX_EXPORT_API
uint32_t
FcitxKeySymToUnicode (FcitxKeySym keyval)
{
    int min = 0;
    int max = sizeof (gdk_keysym_to_unicode_tab) / sizeof(gdk_keysym_to_unicode_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((keyval >= 0x0020 && keyval <= 0x007e) ||
            (keyval >= 0x00a0 && keyval <= 0x00ff))
        return keyval;

    /* Also check for directly encoded 24-bit UCS characters:
    */
    if ((keyval & 0xff000000) == 0x01000000)
        return keyval & 0x00ffffff;

    /* binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_keysym_to_unicode_tab[mid].keysym < keyval)
            min = mid + 1;
        else if (gdk_keysym_to_unicode_tab[mid].keysym > keyval)
            max = mid - 1;
        else {
            /* found it */
            return gdk_keysym_to_unicode_tab[mid].ucs;
        }
    }

    /* No matching Unicode value found */
    return 0;
}

FCITX_EXPORT_API
FcitxKeySym
FcitxUnicodeToKeySym (uint32_t wc)
{
    int min = 0;
    int max = sizeof(gdk_unicode_to_keysym_tab) / sizeof(gdk_unicode_to_keysym_tab[0]) - 1;
    int mid;

    /* First check for Latin-1 characters (1:1 mapping) */
    if ((wc >= 0x0020 && wc <= 0x007e) ||
            (wc >= 0x00a0 && wc <= 0x00ff))
        return wc;

    /* Binary search in table */
    while (max >= min) {
        mid = (min + max) / 2;
        if (gdk_unicode_to_keysym_tab[mid].ucs < wc)
            min = mid + 1;
        else if (gdk_unicode_to_keysym_tab[mid].ucs > wc)
            max = mid - 1;
        else {
            /* found it */
            return gdk_unicode_to_keysym_tab[mid].keysym;
        }
    }

    /*
    * No matching keysym value found, return Unicode value plus 0x01000000
    * (a convention introduced in the UTF-8 work on xterm).
    */
    return wc | 0x01000000;
}

static int FcitxHotkeyGetKeyList(const char *strKey);
static char *FcitxHotkeyGetKeyListString(int key);

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyDigit(FcitxKeySym sym, unsigned int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_0 && sym <= FcitxKey_9)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyUAZ(FcitxKeySym sym, unsigned int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_A && sym <= FcitxKey_Z)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeySimple(FcitxKeySym sym, unsigned int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_space && sym <= FcitxKey_asciitilde)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyLAZ(FcitxKeySym sym, unsigned int state)
{
    if (state)
        return false;

    if (sym >= FcitxKey_a && sym <= FcitxKey_z)
        return true;

    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsKey(FcitxKeySym sym, unsigned int state, FcitxKeySym symcmp, unsigned int statecmp)
{
    FcitxHotkey key[2] = { {0, symcmp, statecmp }, {0, 0, 0} };
    return FcitxHotkeyIsHotKey(sym ,state, key);
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKey(FcitxKeySym sym, unsigned int state, const FcitxHotkey * hotkey)
{
    state &= FcitxKeyState_Ctrl_Alt_Shift | FcitxKeyState_Super;
    if (hotkey[0].sym && sym == hotkey[0].sym && (hotkey[0].state == state))
        return true;
    if (hotkey[1].sym && sym == hotkey[1].sym && (hotkey[1].state == state))
        return true;
    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotkeyCursorMove(FcitxKeySym sym, unsigned int state)
{
    if ((
                sym == FcitxKey_Left
                || sym == FcitxKey_Right
                || sym == FcitxKey_Up
                || sym == FcitxKey_Down
                || sym == FcitxKey_Page_Up
                || sym == FcitxKey_Page_Down
                || sym == FcitxKey_Home
                || sym == FcitxKey_End
            ) && (
                state == FcitxKeyState_Ctrl
                || state == FcitxKeyState_Ctrl_Shift
                || state == FcitxKeyState_Shift
                || state == FcitxKeyState_None
            )

       ) {
        return true;
    }
    return false;
}

FCITX_EXPORT_API
boolean FcitxHotkeyIsHotKeyModifierCombine(FcitxKeySym sym, unsigned int state)
{
    FCITX_UNUSED(state);
    if (sym == FcitxKey_Control_L || sym == FcitxKey_Control_R ||
        sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R)
        return true;
    return false;
}

/*
 * Do some custom process
 */
FCITX_EXPORT_API
void FcitxHotkeyGetKey(FcitxKeySym keysym, unsigned int iKeyState, FcitxKeySym* outk, unsigned int* outs)
{
    /* key state != 0 */
    if (iKeyState) {
        if (iKeyState != FcitxKeyState_Shift && FcitxHotkeyIsHotKeyLAZ(keysym, 0))
            keysym = keysym + FcitxKey_A - FcitxKey_a;
        /*
         * alt shift 1 shoud be alt + !
         * shift+s should be S
         */

        if (FcitxHotkeyIsHotKeyLAZ(keysym, 0) || FcitxHotkeyIsHotKeyUAZ(keysym, 0)) {
            if (iKeyState == FcitxKeyState_Shift)
                iKeyState &= ~FcitxKeyState_Shift;
        }
        else {
            if ((iKeyState & FcitxKeyState_Shift)
                && (((FcitxHotkeyIsHotKeySimple(keysym, 0) || FcitxKeySymToUnicode(keysym) != 0)
                    && keysym != FcitxKey_space && keysym != FcitxKey_Return)
                    || (keysym >= FcitxKey_KP_0 && keysym <= FcitxKey_KP_9)))
                iKeyState &= ~FcitxKeyState_Shift;
        }
    }

    if (keysym == FcitxKey_ISO_Left_Tab)
        keysym = FcitxKey_Tab;

    *outk = keysym;

    *outs = iKeyState;
}


FCITX_EXPORT_API
char* FcitxHotkeyGetKeyString(FcitxKeySym sym, unsigned int state)
{
    char *str;
    size_t len = 0;

    if (state & FcitxKeyState_Ctrl)
        len += strlen("CTRL_");

    if (state & FcitxKeyState_Alt)
        len += strlen("ALT_");

    if (state & FcitxKeyState_Shift)
        len += strlen("SHIFT_");

    if (state & FcitxKeyState_Super)
        len += strlen("SUPER_");

    char *key = FcitxHotkeyGetKeyListString(sym);

    if (!key)
        return NULL;

    len += strlen(key);

    str = fcitx_utils_malloc0(sizeof(char) * (len + 1));

    if (state & FcitxKeyState_Ctrl)
        strcat(str, "CTRL_");

    if (state & FcitxKeyState_Alt)
        strcat(str, "ALT_");

    if (state & FcitxKeyState_Shift)
        strcat(str, "SHIFT_");

    if (state & FcitxKeyState_Super)
        strcat(str, "SUPER_");

    strcat(str, key);

    free(key);

    return str;
}

FCITX_EXPORT_API
boolean FcitxHotkeyParseKey(const char *strKey, FcitxKeySym* sym, unsigned int* state)
{
    const char      *p;
    int             iKey;
    int             iKeyState = 0;

    p = strKey;

    if (strstr(p, "CTRL_")) {
        iKeyState |= FcitxKeyState_Ctrl;
        p += strlen("CTRL_");
    }

    if (strstr(p, "ALT_")) {
        iKeyState |= FcitxKeyState_Alt;
        p += strlen("ALT_");
    }

    if (strstr(strKey, "SHIFT_")) {
        iKeyState |= FcitxKeyState_Shift;
        p += strlen("SHIFT_");
    }

    if (strstr(strKey, "SUPER_")) {
        iKeyState |= FcitxKeyState_Super;
        p += strlen("SUPER_");
    }

    iKey = FcitxHotkeyGetKeyList(p);

    if (iKey == -1)
        return false;

    *sym = iKey;

    *state = iKeyState;

    return true;
}

FCITX_EXPORT_API
int FcitxHotkeyGetKeyList(const char *strKey)
{
    int             i;

    i = 0;

    for (;;) {
        if (!keyList[i].code)
            break;

        if (!strcmp(strKey, keyList[i].strKey))
            return keyList[i].code;

        i++;
    }

    if (strlen(strKey) == 1)
        return strKey[0];

    return -1;
}

char *FcitxHotkeyGetKeyListString(int key)
{
    if (key > FcitxKey_space && key <= FcitxKey_asciitilde) {
        char *p;
        p = malloc(sizeof(char) * 2);
        p[0] = key;
        p[1] = '\0';
        return p;
    }

    int             i;

    i = 0;

    for (;;) {
        if (!keyList[i].code)
            break;

        if (keyList[i].code == key)
            return strdup(keyList[i].strKey);

        i++;
    }

    return NULL;

}

FCITX_EXPORT_API
void FcitxHotkeySetKey(const char *str, FcitxHotkey * hotkey)
{
    char           *p;
    char           *strKey;
    int             i = 0, j = 0, k;

    char* strKeys = fcitx_utils_trim(str);
    p = strKeys;

    for (k = 0; k < 2; k++) {
        FcitxKeySym sym;
        unsigned int state;
        i = 0;

        while (p[i] != ' ' && p[i] != '\0')
            i++;

        strKey = strndup(p, i);

        strKey[i] = '\0';

        if (FcitxHotkeyParseKey(strKey, &sym, &state)) {
            hotkey[j].sym = sym;
            hotkey[j].state = state;
            hotkey[j].desc = fcitx_utils_trim(strKey);
            j ++;
        }

        free(strKey);

        if (p[i] == '\0')
            break;

        p = &p[i + 1];
    }

    for (; j < 2; j++) {
        hotkey[j].sym = 0;
        hotkey[j].state = 0;
        hotkey[j].desc = NULL;
    }

    free(strKeys);
}

FCITX_EXPORT_API
FcitxKeySym FcitxHotkeyPadToMain(FcitxKeySym sym)
{
#define PAD_TO_MAIN(keypad, keymain) case keypad: return keymain
    switch (sym) {
        PAD_TO_MAIN(FcitxKey_KP_Space, FcitxKey_space);
        PAD_TO_MAIN(FcitxKey_KP_Tab, FcitxKey_Tab);
        PAD_TO_MAIN(FcitxKey_KP_Enter, FcitxKey_Return);
        PAD_TO_MAIN(FcitxKey_KP_F1, FcitxKey_F1);
        PAD_TO_MAIN(FcitxKey_KP_F2, FcitxKey_F2);
        PAD_TO_MAIN(FcitxKey_KP_F3, FcitxKey_F3);
        PAD_TO_MAIN(FcitxKey_KP_F4, FcitxKey_F4);
        PAD_TO_MAIN(FcitxKey_KP_Home, FcitxKey_Home);
        PAD_TO_MAIN(FcitxKey_KP_Left, FcitxKey_Left);
        PAD_TO_MAIN(FcitxKey_KP_Up, FcitxKey_Up);
        PAD_TO_MAIN(FcitxKey_KP_Right, FcitxKey_Right);
        PAD_TO_MAIN(FcitxKey_KP_Down, FcitxKey_Down);
        PAD_TO_MAIN(FcitxKey_KP_Page_Up, FcitxKey_Page_Up);
        PAD_TO_MAIN(FcitxKey_KP_Page_Down, FcitxKey_Page_Down);
        PAD_TO_MAIN(FcitxKey_KP_End, FcitxKey_End);
        PAD_TO_MAIN(FcitxKey_KP_Begin, FcitxKey_Begin);
        PAD_TO_MAIN(FcitxKey_KP_Insert, FcitxKey_Insert);
        PAD_TO_MAIN(FcitxKey_KP_Delete, FcitxKey_Delete);
        PAD_TO_MAIN(FcitxKey_KP_Multiply, FcitxKey_asterisk);
        PAD_TO_MAIN(FcitxKey_KP_Add, FcitxKey_plus);
        PAD_TO_MAIN(FcitxKey_KP_Separator, FcitxKey_comma);
        PAD_TO_MAIN(FcitxKey_KP_Subtract, FcitxKey_minus);
        PAD_TO_MAIN(FcitxKey_KP_Decimal, FcitxKey_period);
        PAD_TO_MAIN(FcitxKey_KP_Divide, FcitxKey_slash);

        PAD_TO_MAIN(FcitxKey_KP_0, FcitxKey_0);
        PAD_TO_MAIN(FcitxKey_KP_1, FcitxKey_1);
        PAD_TO_MAIN(FcitxKey_KP_2, FcitxKey_2);
        PAD_TO_MAIN(FcitxKey_KP_3, FcitxKey_3);
        PAD_TO_MAIN(FcitxKey_KP_4, FcitxKey_4);
        PAD_TO_MAIN(FcitxKey_KP_5, FcitxKey_5);
        PAD_TO_MAIN(FcitxKey_KP_6, FcitxKey_6);
        PAD_TO_MAIN(FcitxKey_KP_7, FcitxKey_7);
        PAD_TO_MAIN(FcitxKey_KP_8, FcitxKey_8);
        PAD_TO_MAIN(FcitxKey_KP_9, FcitxKey_9);

        PAD_TO_MAIN(FcitxKey_KP_Equal, FcitxKey_equal);
    default:
        break;
    }
#undef PAD_TO_MAIN
    return sym;
}

FCITX_EXPORT_API
void FcitxHotkeyFree(FcitxHotkey* hotkey)
{
    if (hotkey[0].desc)
        free(hotkey[0].desc);
    if (hotkey[1].desc)
        free(hotkey[1].desc);
    hotkey[0].desc = NULL;
    hotkey[1].desc = NULL;
}

// kate: indent-mode cstyle; space-indent on; indent-width 0;
