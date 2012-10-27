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

#include "fcitx/fcitx.h"
#include "PYFA.h"
#include "pyconfig.h"

#include <stdio.h>
#include "fcitx-utils/log.h"
#include <string.h>
#include <fcitx-utils/utils.h>

const MHPY_TEMPLATE  MHPY_C_TEMPLATE[] = {    //韵母
    //{"an","ang"},
    {"CD"}
    ,
    //{"en","eng"},
    {"HI"}
    ,
    //{"ian","iang"},
    {"LM"}
    ,
    //{"in","ing"},
    {"PQ"}
    ,
    //{"u","ou"},
    {"VW"}
    ,
    //{"uan","uang"},
    {"Za"}
    ,
    //{"v", "u"},
    {"fW"}
    ,

    {"\0"}
};

const MHPY_TEMPLATE MHPY_S_TEMPLATE[] = {    //声母
    //{"c","ch"},
    {"bc"}
    ,
    //{"f","h"},
    {"TV"}
    ,
    //{"l","n"},
    {"OQ"}
    ,
    //{"s","sh"},
    {"GH"}
    ,
    //{"z","zh"},
    {"AB"}
    ,
    //{"an","ang"}
    {"fg"}
    ,

    {"\0"}
};

//其中增加了那些不是标准的拼音，但模糊输入中需要使用的拼音组合
const PYTABLE_TEMPLATE  PYTable_template[] = {
    {"zuo", PYTABLE_NONE}
    ,
    {"zun", PYTABLE_NONE}
    ,
    {"zui", PYTABLE_NONE}
    ,
    {"zuagn", PYTABLE_NG_GN}
    ,
    {"zuang", PYTABLE_UAN_UANG}
    ,
    {"zuagn", PYTABLE_NG_GN}
    ,
    {"zuang", PYTABLE_Z_ZH}
    ,
    {"zuan", PYTABLE_NONE}
    ,
    {"zua", PYTABLE_Z_ZH}
    ,
    {"zu", PYTABLE_NONE}
    ,
    {"zou", PYTABLE_NONE}
    ,
    {"zogn", PYTABLE_NG_GN}
    ,
    {"zong", PYTABLE_NONE}
    ,
    {"zi", PYTABLE_NONE}
    ,
    {"zhuo", PYTABLE_NONE}
    ,
    {"zhun", PYTABLE_NONE}
    ,
    {"zhui", PYTABLE_NONE}
    ,
    {"zhuagn", PYTABLE_NG_GN}
    ,
    {"zhuang", PYTABLE_NONE}
    ,
    {"zhuan", PYTABLE_NONE}
    ,
    {"zhuai", PYTABLE_NONE}
    ,
    {"zhua", PYTABLE_NONE}
    ,
    {"zhu", PYTABLE_NONE}
    ,
    {"zhou", PYTABLE_NONE}
    ,
    {"zhogn", PYTABLE_NG_GN}
    ,
    {"zhong", PYTABLE_NONE}
    ,
    {"zhi", PYTABLE_NONE}
    ,
    {"zhegn", PYTABLE_NG_GN}
    ,
    {"zheng", PYTABLE_NONE}
    ,
    {"zhen", PYTABLE_NONE}
    ,
    {"zhe", PYTABLE_NONE}
    ,
    {"zhao", PYTABLE_NONE}
    ,
    {"zhagn", PYTABLE_NG_GN}
    ,
    {"zhang", PYTABLE_NONE}
    ,
    {"zhan", PYTABLE_NONE}
    ,
    {"zhai", PYTABLE_NONE}
    ,
    {"zha", PYTABLE_NONE}
    ,
    {"zegn", PYTABLE_NG_GN}
    ,
    {"zeng", PYTABLE_NONE}
    ,
    {"zen", PYTABLE_NONE}
    ,
    {"zei", PYTABLE_NONE}
    ,
    {"ze", PYTABLE_NONE}
    ,
    {"zao", PYTABLE_NONE}
    ,
    {"zagn", PYTABLE_NG_GN}
    ,
    {"zang", PYTABLE_NONE}
    ,
    {"zan", PYTABLE_NONE}
    ,
    {"zai", PYTABLE_NONE}
    ,
    {"za", PYTABLE_NONE}
    ,
    {"yun", PYTABLE_NONE}
    ,
    {"yue", PYTABLE_NONE}
    ,
    {"yuagn", PYTABLE_NG_GN}
    ,
    {"yuang", PYTABLE_UAN_UANG}
    ,
    {"yuan", PYTABLE_NONE}
    ,
    {"yu", PYTABLE_NONE}
    ,
    {"you", PYTABLE_NONE}
    ,
    {"yogn", PYTABLE_NG_GN}
    ,
    {"yong", PYTABLE_NONE}
    ,
    {"yo", PYTABLE_NONE}
    ,
    {"yign", PYTABLE_NG_GN}
    ,
    {"ying", PYTABLE_NONE}
    ,
    {"yin", PYTABLE_NONE}
    ,
    {"yi", PYTABLE_NONE}
    ,
    {"ye", PYTABLE_NONE}
    ,
    {"yao", PYTABLE_NONE}
    ,
    {"yagn", PYTABLE_NG_GN}
    ,
    {"yang", PYTABLE_NONE}
    ,
    {"yan", PYTABLE_NONE}
    ,
    {"ya", PYTABLE_NONE}
    ,
    {"xun", PYTABLE_NONE}
    ,
    {"xue", PYTABLE_NONE}
    ,
    {"xuagn", PYTABLE_NG_GN}
    ,
    {"xuang", PYTABLE_UAN_UANG}
    ,
    {"xuan", PYTABLE_NONE}
    ,
    {"xu", PYTABLE_NONE}
    ,
    {"xv", PYTABLE_V_U}
    ,
    {"xou", PYTABLE_U_OU}
    ,
    {"xiu", PYTABLE_NONE}
    ,
    {"xiogn", PYTABLE_NG_GN}
    ,
    {"xiong", PYTABLE_NONE}
    ,
    {"xign", PYTABLE_NG_GN}
    ,
    {"xing", PYTABLE_NONE}
    ,
    {"xin", PYTABLE_NONE}
    ,
    {"xie", PYTABLE_NONE}
    ,
    {"xiao", PYTABLE_NONE}
    ,
    {"xiagn", PYTABLE_NG_GN}
    ,
    {"xiang", PYTABLE_NONE}
    ,
    {"xian", PYTABLE_NONE}
    ,
    {"xia", PYTABLE_NONE}
    ,
    {"xi", PYTABLE_NONE}
    ,
    {"wu", PYTABLE_NONE}
    ,
    {"wo", PYTABLE_NONE}
    ,
    {"wegn", PYTABLE_NG_GN}
    ,
    {"weng", PYTABLE_NONE}
    ,
    {"wen", PYTABLE_NONE}
    ,
    {"wei", PYTABLE_NONE}
    ,
    {"wagn", PYTABLE_NG_GN}
    ,
    {"wang", PYTABLE_NONE}
    ,
    {"wan", PYTABLE_NONE}
    ,
    {"wai", PYTABLE_NONE}
    ,
    {"wa", PYTABLE_NONE}
    ,
    {"tuo", PYTABLE_NONE}
    ,
    {"tun", PYTABLE_NONE}
    ,
    {"tui", PYTABLE_NONE}
    ,
    {"tuagn", PYTABLE_NG_GN}
    ,
    {"tuang", PYTABLE_UAN_UANG}
    ,
    {"tuan", PYTABLE_NONE}
    ,
    {"tu", PYTABLE_NONE}
    ,
    {"tou", PYTABLE_NONE}
    ,
    {"togn", PYTABLE_NG_GN}
    ,
    {"tong", PYTABLE_NONE}
    ,
    {"tign", PYTABLE_NG_GN}
    ,
    {"ting", PYTABLE_NONE}
    ,
    {"tin", PYTABLE_IN_ING}
    ,
    {"tie", PYTABLE_NONE}
    ,
    {"tiao", PYTABLE_NONE}
    ,
    {"tiagn", PYTABLE_NG_GN}
    ,
    {"tiang", PYTABLE_IAN_IANG}
    ,
    {"tian", PYTABLE_NONE}
    ,
    {"ti", PYTABLE_NONE}
    ,
    {"tegn", PYTABLE_NG_GN}
    ,
    {"teng", PYTABLE_NONE}
    ,
    {"ten", PYTABLE_EN_ENG}
    ,
    {"tei", PYTABLE_NONE}
    ,
    {"te", PYTABLE_NONE}
    ,
    {"tao", PYTABLE_NONE}
    ,
    {"tagn", PYTABLE_NG_GN}
    ,
    {"tang", PYTABLE_NONE}
    ,
    {"tan", PYTABLE_NONE}
    ,
    {"tai", PYTABLE_NONE}
    ,
    {"ta", PYTABLE_NONE}
    ,
    {"suo", PYTABLE_NONE}
    ,
    {"sun", PYTABLE_NONE}
    ,
    {"sui", PYTABLE_NONE}
    ,
    {"suagn", PYTABLE_NG_GN}
    ,
    {"suang", PYTABLE_S_SH}
    ,
    {"suagn", PYTABLE_NG_GN}
    ,
    {"suang", PYTABLE_UAN_UANG}
    ,
    {"suan", PYTABLE_NONE}
    ,
    {"sua", PYTABLE_S_SH}
    ,
    {"su", PYTABLE_NONE}
    ,
    {"sou", PYTABLE_NONE}
    ,
    {"sogn", PYTABLE_NG_GN}
    ,
    {"song", PYTABLE_NONE}
    ,
    {"si", PYTABLE_NONE}
    ,
    {"shuo", PYTABLE_NONE}
    ,
    {"shun", PYTABLE_NONE}
    ,
    {"shui", PYTABLE_NONE}
    ,
    {"shuagn", PYTABLE_NG_GN}
    ,
    {"shuang", PYTABLE_NONE}
    ,
    {"shuan", PYTABLE_NONE}
    ,
    {"shuai", PYTABLE_NONE}
    ,
    {"shua", PYTABLE_NONE}
    ,
    {"shu", PYTABLE_NONE}
    ,
    {"shou", PYTABLE_NONE}
    ,
    {"shi", PYTABLE_NONE}
    ,
    {"shegn", PYTABLE_NG_GN}
    ,
    {"sheng", PYTABLE_NONE}
    ,
    {"shen", PYTABLE_NONE}
    ,
    {"shei", PYTABLE_NONE}
    ,
    {"she", PYTABLE_NONE}
    ,
    {"shao", PYTABLE_NONE}
    ,
    {"shagn", PYTABLE_NG_GN}
    ,
    {"shang", PYTABLE_NONE}
    ,
    {"shan", PYTABLE_NONE}
    ,
    {"shai", PYTABLE_NONE}
    ,
    {"sha", PYTABLE_NONE}
    ,
    {"segn", PYTABLE_NG_GN}
    ,
    {"seng", PYTABLE_NONE}
    ,
    {"sen", PYTABLE_NONE}
    ,
    {"se", PYTABLE_NONE}
    ,
    {"sao", PYTABLE_NONE}
    ,
    {"sagn", PYTABLE_NG_GN}
    ,
    {"sang", PYTABLE_NONE}
    ,
    {"san", PYTABLE_NONE}
    ,
    {"sai", PYTABLE_NONE}
    ,
    {"sa", PYTABLE_NONE}
    ,
    {"ruo", PYTABLE_NONE}
    ,
    {"run", PYTABLE_NONE}
    ,
    {"rui", PYTABLE_NONE}
    ,
    {"ruagn", PYTABLE_NG_GN}
    ,
    {"ruang", PYTABLE_UAN_UANG}
    ,
    {"ruan", PYTABLE_NONE}
    ,
    {"ru", PYTABLE_NONE}
    ,
    {"rou", PYTABLE_NONE}
    ,
    {"rogn", PYTABLE_NG_GN}
    ,
    {"rong", PYTABLE_NONE}
    ,
    {"ri", PYTABLE_NONE}
    ,
    {"regn", PYTABLE_NG_GN}
    ,
    {"reng", PYTABLE_NONE}
    ,
    {"ren", PYTABLE_NONE}
    ,
    {"re", PYTABLE_NONE}
    ,
    {"rao", PYTABLE_NONE}
    ,
    {"ragn", PYTABLE_NG_GN}
    ,
    {"rang", PYTABLE_NONE}
    ,
    {"ran", PYTABLE_NONE}
    ,
    {"qun", PYTABLE_NONE}
    ,
    {"que", PYTABLE_NONE}
    ,
    {"quagn", PYTABLE_NG_GN}
    ,
    {"quang", PYTABLE_UAN_UANG}
    ,
    {"quan", PYTABLE_NONE}
    ,
    {"qu", PYTABLE_NONE}
    ,
    {"qv", PYTABLE_V_U}
    ,
    {"qiu", PYTABLE_NONE}
    ,
    {"qiogn", PYTABLE_NG_GN}
    ,
    {"qiong", PYTABLE_NONE}
    ,
    {"qign", PYTABLE_NG_GN}
    ,
    {"qing", PYTABLE_NONE}
    ,
    {"qin", PYTABLE_NONE}
    ,
    {"qie", PYTABLE_NONE}
    ,
    {"qiao", PYTABLE_NONE}
    ,
    {"qiagn", PYTABLE_NG_GN}
    ,
    {"qiang", PYTABLE_NONE}
    ,
    {"qian", PYTABLE_NONE}
    ,
    {"qia", PYTABLE_NONE}
    ,
    {"qi", PYTABLE_NONE}
    ,
    {"pu", PYTABLE_NONE}
    ,
    {"pou", PYTABLE_NONE}
    ,
    {"po", PYTABLE_NONE}
    ,
    {"pign", PYTABLE_NG_GN}
    ,
    {"ping", PYTABLE_NONE}
    ,
    {"pin", PYTABLE_NONE}
    ,
    {"pie", PYTABLE_NONE}
    ,
    {"piao", PYTABLE_NONE}
    ,
    {"piagn", PYTABLE_NG_GN}
    ,
    {"piang", PYTABLE_IAN_IANG}
    ,
    {"pian", PYTABLE_NONE}
    ,
    {"pi", PYTABLE_NONE}
    ,
    {"pegn", PYTABLE_NG_GN}
    ,
    {"peng", PYTABLE_NONE}
    ,
    {"pen", PYTABLE_NONE}
    ,
    {"pei", PYTABLE_NONE}
    ,
    {"pao", PYTABLE_NONE}
    ,
    {"pagn", PYTABLE_NG_GN}
    ,
    {"pang", PYTABLE_NONE}
    ,
    {"pan", PYTABLE_NONE}
    ,
    {"pai", PYTABLE_NONE}
    ,
    {"pa", PYTABLE_NONE}
    ,
    {"ou", PYTABLE_NONE}
    ,
    {"o", PYTABLE_NONE}
    ,
    {"nve", PYTABLE_NONE}
    ,
    {"nv", PYTABLE_NONE}
    ,
    {"nuo", PYTABLE_NONE}
    ,
    {"nue", PYTABLE_NONE}
    ,
    {"nuagn", PYTABLE_NG_GN}
    ,
    {"nuang", PYTABLE_UAN_UANG}
    ,
    {"nuagn", PYTABLE_NG_GN}
    ,
    {"nuang", PYTABLE_L_N}
    ,
    {"nuan", PYTABLE_NONE}
    ,
    {"nu", PYTABLE_NONE}
    ,
    {"nou", PYTABLE_NONE}
    ,
    {"nogn", PYTABLE_NG_GN}
    ,
    {"nong", PYTABLE_NONE}
    ,
    {"niu", PYTABLE_NONE}
    ,
    {"nign", PYTABLE_NG_GN}
    ,
    {"ning", PYTABLE_NONE}
    ,
    {"nin", PYTABLE_NONE}
    ,
    {"nie", PYTABLE_NONE}
    ,
    {"niao", PYTABLE_NONE}
    ,
    {"niagn", PYTABLE_NG_GN}
    ,
    {"niang", PYTABLE_NONE}
    ,
    {"nian", PYTABLE_NONE}
    ,
    {"ni", PYTABLE_NONE}
    ,
    {"ng", PYTABLE_NONE}
    ,
    {"negn", PYTABLE_NG_GN}
    ,
    {"neng", PYTABLE_NONE}
    ,
    {"nen", PYTABLE_NONE}
    ,
    {"nei", PYTABLE_NONE}
    ,
    {"ne", PYTABLE_NONE}
    ,
    {"nao", PYTABLE_NONE}
    ,
    {"nagn", PYTABLE_NG_GN}
    ,
    {"nang", PYTABLE_NONE}
    ,
    {"nan", PYTABLE_NONE}
    ,
    {"nai", PYTABLE_NONE}
    ,
    {"na", PYTABLE_NONE}
    ,
    {"n", PYTABLE_NONE}
    ,
    {"mu", PYTABLE_NONE}
    ,
    {"mou", PYTABLE_NONE}
    ,
    {"mo", PYTABLE_NONE}
    ,
    {"miu", PYTABLE_NONE}
    ,
    {"mign", PYTABLE_NG_GN}
    ,
    {"ming", PYTABLE_NONE}
    ,
    {"min", PYTABLE_NONE}
    ,
    {"mie", PYTABLE_NONE}
    ,
    {"miao", PYTABLE_NONE}
    ,
    {"miagn", PYTABLE_NG_GN}
    ,
    {"miang", PYTABLE_IAN_IANG}
    ,
    {"mian", PYTABLE_NONE}
    ,
    {"mi", PYTABLE_NONE}
    ,
    {"megn", PYTABLE_NG_GN}
    ,
    {"meng", PYTABLE_NONE}
    ,
    {"men", PYTABLE_NONE}
    ,
    {"mei", PYTABLE_NONE}
    ,
    {"me", PYTABLE_NONE}
    ,
    {"mao", PYTABLE_NONE}
    ,
    {"magn", PYTABLE_NG_GN}
    ,
    {"mang", PYTABLE_NONE}
    ,
    {"man", PYTABLE_NONE}
    ,
    {"mai", PYTABLE_NONE}
    ,
    {"ma", PYTABLE_NONE}
    ,
    {"m", PYTABLE_NONE}
    ,
    {"lve", PYTABLE_NONE}
    ,
    {"lv", PYTABLE_NONE}
    ,
    {"luo", PYTABLE_NONE}
    ,
    {"lun", PYTABLE_NONE}
    ,
    {"lue", PYTABLE_NONE}
    ,
    {"luagn", PYTABLE_NG_GN}
    ,
    {"luang", PYTABLE_UAN_UANG}
    ,
    {"luagn", PYTABLE_NG_GN}
    ,
    {"luang", PYTABLE_L_N}
    ,
    {"luan", PYTABLE_NONE}
    ,
    {"lu", PYTABLE_NONE}
    ,
    {"lou", PYTABLE_NONE}
    ,
    {"logn", PYTABLE_NG_GN}
    ,
    {"long", PYTABLE_NONE}
    ,
    {"lo", PYTABLE_NONE}
    ,
    {"liu", PYTABLE_NONE}
    ,
    {"lign", PYTABLE_NG_GN}
    ,
    {"ling", PYTABLE_NONE}
    ,
    {"lin", PYTABLE_NONE}
    ,
    {"lie", PYTABLE_NONE}
    ,
    {"liao", PYTABLE_NONE}
    ,
    {"liagn", PYTABLE_NG_GN}
    ,
    {"liang", PYTABLE_NONE}
    ,
    {"lian", PYTABLE_NONE}
    ,
    {"lia", PYTABLE_NONE}
    ,
    {"li", PYTABLE_NONE}
    ,
    {"legn", PYTABLE_NG_GN}
    ,
    {"leng", PYTABLE_NONE}
    ,
    {"len", PYTABLE_EN_ENG}
    ,
    {"lei", PYTABLE_NONE}
    ,
    {"le", PYTABLE_NONE}
    ,
    {"lao", PYTABLE_NONE}
    ,
    {"lagn", PYTABLE_NG_GN}
    ,
    {"lang", PYTABLE_NONE}
    ,
    {"lan", PYTABLE_NONE}
    ,
    {"lai", PYTABLE_NONE}
    ,
    {"la", PYTABLE_NONE}
    ,
    {"kuo", PYTABLE_NONE}
    ,
    {"kun", PYTABLE_NONE}
    ,
    {"kui", PYTABLE_NONE}
    ,
    {"kuagn", PYTABLE_NG_GN}
    ,
    {"kuang", PYTABLE_NONE}
    ,
    {"kuan", PYTABLE_NONE}
    ,
    {"kuai", PYTABLE_NONE}
    ,
    {"kua", PYTABLE_NONE}
    ,
    {"ku", PYTABLE_NONE}
    ,
    {"kou", PYTABLE_NONE}
    ,
    {"kogn", PYTABLE_NG_GN}
    ,
    {"kong", PYTABLE_NONE}
    ,
    {"kegn", PYTABLE_NG_GN}
    ,
    {"keng", PYTABLE_NONE}
    ,
    {"ken", PYTABLE_NONE}
    ,
    {"kei", PYTABLE_NONE}
    ,
    {"ke", PYTABLE_NONE}
    ,
    {"kao", PYTABLE_NONE}
    ,
    {"kagn", PYTABLE_NG_GN}
    ,
    {"kang", PYTABLE_NONE}
    ,
    {"kan", PYTABLE_NONE}
    ,
    {"kai", PYTABLE_NONE}
    ,
    {"ka", PYTABLE_NONE}
    ,
    {"jun", PYTABLE_NONE}
    ,
    {"jue", PYTABLE_NONE}
    ,
    {"juagn", PYTABLE_NG_GN}
    ,
    {"juang", PYTABLE_UAN_UANG}
    ,
    {"juan", PYTABLE_NONE}
    ,
    {"ju", PYTABLE_NONE}
    ,
    {"jv", PYTABLE_V_U}
    ,
    {"jiu", PYTABLE_NONE}
    ,
    {"jiogn", PYTABLE_NG_GN}
    ,
    {"jiong", PYTABLE_NONE}
    ,
    {"jign", PYTABLE_NG_GN}
    ,
    {"jing", PYTABLE_NONE}
    ,
    {"jin", PYTABLE_NONE}
    ,
    {"jie", PYTABLE_NONE}
    ,
    {"jiao", PYTABLE_NONE}
    ,
    {"jiagn", PYTABLE_NG_GN}
    ,
    {"jiang", PYTABLE_NONE}
    ,
    {"jian", PYTABLE_NONE}
    ,
    {"jia", PYTABLE_NONE}
    ,
    {"ji", PYTABLE_NONE}
    ,
    {"huo", PYTABLE_NONE}
    ,
    {"hun", PYTABLE_NONE}
    ,
    {"hui", PYTABLE_NONE}
    ,
    {"huagn", PYTABLE_NG_GN}
    ,
    {"huang", PYTABLE_NONE}
    ,
    {"huan", PYTABLE_NONE}
    ,
    {"huai", PYTABLE_NONE}
    ,
    {"hua", PYTABLE_NONE}
    ,
    {"hu", PYTABLE_NONE}
    ,
    {"hou", PYTABLE_NONE}
    ,
    {"hogn", PYTABLE_NG_GN}
    ,
    {"hong", PYTABLE_NONE}
    ,
    {"hegn", PYTABLE_NG_GN}
    ,
    {"heng", PYTABLE_NONE}
    ,
    {"hen", PYTABLE_NONE}
    ,
    {"hei", PYTABLE_NONE}
    ,
    {"he", PYTABLE_NONE}
    ,
    {"hao", PYTABLE_NONE}
    ,
    {"hagn", PYTABLE_NG_GN}
    ,
    {"hang", PYTABLE_NONE}
    ,
    {"han", PYTABLE_NONE}
    ,
    {"hai", PYTABLE_NONE}
    ,
    {"ha", PYTABLE_NONE}
    ,
    {"guo", PYTABLE_NONE}
    ,
    {"gun", PYTABLE_NONE}
    ,
    {"gui", PYTABLE_NONE}
    ,
    {"guagn", PYTABLE_NG_GN}
    ,
    {"guang", PYTABLE_NONE}
    ,
    {"guan", PYTABLE_NONE}
    ,
    {"guai", PYTABLE_NONE}
    ,
    {"gua", PYTABLE_NONE}
    ,
    {"gu", PYTABLE_NONE}
    ,
    {"gou", PYTABLE_NONE}
    ,
    {"gogn", PYTABLE_NG_GN}
    ,
    {"gong", PYTABLE_NONE}
    ,
    {"gegn", PYTABLE_NG_GN}
    ,
    {"geng", PYTABLE_NONE}
    ,
    {"gen", PYTABLE_NONE}
    ,
    {"gei", PYTABLE_NONE}
    ,
    {"ge", PYTABLE_NONE}
    ,
    {"gao", PYTABLE_NONE}
    ,
    {"gagn", PYTABLE_NG_GN}
    ,
    {"gang", PYTABLE_NONE}
    ,
    {"gan", PYTABLE_NONE}
    ,
    {"gai", PYTABLE_NONE}
    ,
    {"ga", PYTABLE_NONE}
    ,
    {"fu", PYTABLE_NONE}
    ,
    {"fou", PYTABLE_NONE}
    ,
    {"fo", PYTABLE_NONE}
    ,
    {"fegn", PYTABLE_NG_GN}
    ,
    {"feng", PYTABLE_NONE}
    ,
    {"fen", PYTABLE_NONE}
    ,
    {"fei", PYTABLE_NONE}
    ,
    {"fagn", PYTABLE_NG_GN}
    ,
    {"fang", PYTABLE_NONE}
    ,
    {"fan", PYTABLE_NONE}
    ,
    {"fa", PYTABLE_NONE}
    ,
    {"er", PYTABLE_NONE}
    ,
    {"egn", PYTABLE_NG_GN}
    ,
    {"eng", PYTABLE_EN_ENG}
    ,
    {"en", PYTABLE_NONE}
    ,
    {"ei", PYTABLE_NONE}
    ,
    {"e", PYTABLE_NONE}
    ,
    {"duo", PYTABLE_NONE}
    ,
    {"dun", PYTABLE_NONE}
    ,
    {"dui", PYTABLE_NONE}
    ,
    {"duagn", PYTABLE_NG_GN}
    ,
    {"duang", PYTABLE_UAN_UANG}
    ,
    {"duan", PYTABLE_NONE}
    ,
    {"du", PYTABLE_NONE}
    ,
    {"dou", PYTABLE_NONE}
    ,
    {"dogn", PYTABLE_NG_GN}
    ,
    {"dong", PYTABLE_NONE}
    ,
    {"diu", PYTABLE_NONE}
    ,
    {"dign", PYTABLE_NG_GN}
    ,
    {"ding", PYTABLE_NONE}
    ,
    {"din", PYTABLE_IN_ING}
    ,
    {"die", PYTABLE_NONE}
    ,
    {"diao", PYTABLE_NONE}
    ,
    {"diagn", PYTABLE_NG_GN}
    ,
    {"diang", PYTABLE_IAN_IANG}
    ,
    {"dian", PYTABLE_NONE}
    ,
    {"dia", PYTABLE_NONE}
    ,
    {"di", PYTABLE_NONE}
    ,
    {"degn", PYTABLE_NG_GN}
    ,
    {"deng", PYTABLE_NONE}
    ,
    {"den", PYTABLE_NONE}
    ,
    {"dei", PYTABLE_NONE}
    ,
    {"de", PYTABLE_NONE}
    ,
    {"dao", PYTABLE_NONE}
    ,
    {"dagn", PYTABLE_NG_GN}
    ,
    {"dang", PYTABLE_NONE}
    ,
    {"dan", PYTABLE_NONE}
    ,
    {"dai", PYTABLE_NONE}
    ,
    {"da", PYTABLE_NONE}
    ,
    {"cuo", PYTABLE_NONE}
    ,
    {"cun", PYTABLE_NONE}
    ,
    {"cui", PYTABLE_NONE}
    ,
    {"cuagn", PYTABLE_NG_GN}
    ,
    {"cuang", PYTABLE_UAN_UANG}
    ,
    {"cuagn", PYTABLE_NG_GN}
    ,
    {"cuang", PYTABLE_C_CH}
    ,
    {"cuan", PYTABLE_NONE}
    ,
    {"cu", PYTABLE_NONE}
    ,
    {"cou", PYTABLE_NONE}
    ,
    {"cogn", PYTABLE_NG_GN}
    ,
    {"cong", PYTABLE_NONE}
    ,
    {"ci", PYTABLE_NONE}
    ,
    {"chuo", PYTABLE_NONE}
    ,
    {"chun", PYTABLE_NONE}
    ,
    {"chui", PYTABLE_NONE}
    ,
    {"chuagn", PYTABLE_NG_GN}
    ,
    {"chuang", PYTABLE_NONE}
    ,
    {"chuan", PYTABLE_NONE}
    ,
    {"chuai", PYTABLE_NONE}
    ,
    {"chu", PYTABLE_NONE}
    ,
    {"chou", PYTABLE_NONE}
    ,
    {"chogn", PYTABLE_NG_GN}
    ,
    {"chong", PYTABLE_NONE}
    ,
    {"chi", PYTABLE_NONE}
    ,
    {"chegn", PYTABLE_NG_GN}
    ,
    {"cheng", PYTABLE_NONE}
    ,
    {"chen", PYTABLE_NONE}
    ,
    {"che", PYTABLE_NONE}
    ,
    {"chao", PYTABLE_NONE}
    ,
    {"chagn", PYTABLE_NG_GN}
    ,
    {"chang", PYTABLE_NONE}
    ,
    {"chan", PYTABLE_NONE}
    ,
    {"chai", PYTABLE_NONE}
    ,
    {"cha", PYTABLE_NONE}
    ,
    {"cegn", PYTABLE_NG_GN}
    ,
    {"ceng", PYTABLE_NONE}
    ,
    {"cen", PYTABLE_NONE}
    ,
    {"ce", PYTABLE_NONE}
    ,
    {"cao", PYTABLE_NONE}
    ,
    {"cagn", PYTABLE_NG_GN}
    ,
    {"cang", PYTABLE_NONE}
    ,
    {"can", PYTABLE_NONE}
    ,
    {"cai", PYTABLE_NONE}
    ,
    {"ca", PYTABLE_NONE}
    ,
    {"bu", PYTABLE_NONE}
    ,
    {"bo", PYTABLE_NONE}
    ,
    {"bign", PYTABLE_NG_GN}
    ,
    {"bing", PYTABLE_NONE}
    ,
    {"bin", PYTABLE_NONE}
    ,
    {"bie", PYTABLE_NONE}
    ,
    {"biao", PYTABLE_NONE}
    ,
    {"biagn", PYTABLE_NG_GN}
    ,
    {"biang", PYTABLE_IAN_IANG}
    ,
    {"bian", PYTABLE_NONE}
    ,
    {"bi", PYTABLE_NONE}
    ,
    {"begn", PYTABLE_NG_GN}
    ,
    {"beng", PYTABLE_NONE}
    ,
    {"ben", PYTABLE_NONE}
    ,
    {"bei", PYTABLE_NONE}
    ,
    {"bao", PYTABLE_NONE}
    ,
    {"bagn", PYTABLE_NG_GN}
    ,
    {"bang", PYTABLE_NONE}
    ,
    {"ban", PYTABLE_NONE}
    ,
    {"bai", PYTABLE_NONE}
    ,
    {"ba", PYTABLE_NONE}
    ,
    {"ao", PYTABLE_NONE}
    ,
    {"agn", PYTABLE_NG_GN}
    ,
    {"ang", PYTABLE_NONE}
    ,
    {"an", PYTABLE_NONE}
    ,
    {"ai", PYTABLE_NONE}
    ,
    {"a", PYTABLE_NONE}
    ,
    {"\0", PYTABLE_NONE}
};

int GetMHIndex_C(MHPY* MHPY_C, char map)
{
    int             i;

    for (i = 0; MHPY_C[i].strMap[0]; i++) {
        if (map == MHPY_C[i].strMap[0] || map == MHPY_C[i].strMap[1]) {
            if (MHPY_C[i].bMode)
                return i;
            else
                return -1;
        }
    }

    return -1;
}

int GetMHIndex_C2(MHPY* MHPY_C, char map1, char map2)
{
    int             i;

    for (i = 0; MHPY_C[i].strMap[0]; i++) {
        if ((map1 == MHPY_C[i].strMap[0] || map1 == MHPY_C[i].strMap[1])
        && (map2 == MHPY_C[i].strMap[0] || map2 == MHPY_C[i].strMap[1])) {
            if (MHPY_C[i].bMode)
                return i;
            else
                return -1;
        }
    }

    return -1;
}

int GetMHIndex_S2(MHPY* MHPY_S, char map1, char map2, boolean bMode)
{
    int             i;

    for (i = 0; MHPY_S[i].strMap[0]; i++) {
        if ((map1 == MHPY_S[i].strMap[0] || map1 == MHPY_S[i].strMap[1])
        && (map2 == MHPY_S[i].strMap[0] || map2 == MHPY_S[i].strMap[1])) {
            if (MHPY_S[i].bMode || bMode)
                return i;
            else
                return -1;
        }
    }

    return -1;

}

boolean IsZ_C_S(char map)
{
    if (map == 'c' || map == 'H' || map == 'B')
        return true;

    return false;
}

void InitMHPY(MHPY** pMHPY, const MHPY_TEMPLATE* MHPYtemplate)
{
    int iBaseCount = 0;

    while (MHPYtemplate[iBaseCount].strMap[0] != '\0')
        iBaseCount++;

    iBaseCount++;

    *pMHPY = fcitx_utils_malloc0(sizeof(MHPY) * iBaseCount);

    MHPY *mhpy = *pMHPY;

    iBaseCount = 0;

    while (MHPYtemplate[iBaseCount].strMap[0] != '\0') {
        strcpy(mhpy[iBaseCount].strMap, MHPYtemplate[iBaseCount].strMap);
        mhpy[iBaseCount].bMode = false;
        iBaseCount++;
    }
}

void InitPYTable(FcitxPinyinConfig* pyconfig)
{
    int iBaseCount = 0;

    while (PYTable_template[iBaseCount].strPY[0] != '\0')
        iBaseCount++;

    iBaseCount++;

    pyconfig->PYTable = fcitx_utils_malloc0(sizeof(PYTABLE) * iBaseCount);

    iBaseCount = 0;

    while (PYTable_template[iBaseCount].strPY[0] != '\0') {
        strcpy(pyconfig->PYTable[iBaseCount].strPY,
               PYTable_template[iBaseCount].strPY);

        switch (PYTable_template[iBaseCount].control) {

        case PYTABLE_NONE:
            pyconfig->PYTable[iBaseCount].pMH = NULL;
            break;

        case PYTABLE_NG_GN:
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->bMisstypeNGGN;
            break;

        case PYTABLE_V_U:
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[6].bMode;
            break;

        case PYTABLE_AN_ANG: // 0
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[0].bMode;
            break;

        case PYTABLE_EN_ENG: // 1
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[1].bMode;
            break;

        case PYTABLE_IAN_IANG: // 2
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[2].bMode;
            break;

        case PYTABLE_IN_ING: // 3
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[3].bMode;
            break;

        case PYTABLE_U_OU: // 4
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[4].bMode;
            break;

        case PYTABLE_UAN_UANG: // 5
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_C[5].bMode;
            break;

        case PYTABLE_C_CH: // 0
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[0].bMode;
            break;

        case PYTABLE_F_H: // 1
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[1].bMode;
            break;

        case PYTABLE_L_N: // 2
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[2].bMode;
            break;

        case PYTABLE_S_SH: // 3
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[3].bMode;
            break;

        case PYTABLE_Z_ZH: // 4
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[4].bMode;
            break;

        case PYTABLE_AN_ANG_S: //5
            pyconfig->PYTable[iBaseCount].pMH = &pyconfig->MHPY_S[5].bMode;
            break;
        }

        iBaseCount++;
    }
}


// kate: indent-mode cstyle; space-indent on; indent-width 0;
