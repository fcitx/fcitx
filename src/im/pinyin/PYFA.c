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

#include "core/fcitx.h"
#include "fcitx-config/configfile.h"
#include "PYFA.h"

#include <stdio.h>

MHPY            MHPY_C[] = {    //韵母
    //{"an","ang"},
    {"CD", 0}
    ,
    //{"en","eng"},
    {"HI", 0}
    ,
    //{"ian","iang"},
    {"LM", 0}
    ,
    //{"in","ing"},
    {"PQ", 0}
    ,
    //{"u","ou"},
    {"VW", 0}
    ,
    //{"uan","uang"},
    {"Za", 0}
    ,

    {"\0", 0}
};

MHPY            MHPY_S[] = {    //声母
    //{"c","ch"},
    {"bc", 0}
    ,
    //{"f","h"},
    {"TV", 0}
    ,
    //{"l","n"},
    {"OQ", 0}
    ,
    //{"s","sh"},
    {"GH", 0}
    ,
    //{"z","zh"},
    {"AB", 0}
    ,
    //{"an","ang"}
    {"fg", 0}
    ,

    {"\0", 0}
};

//其中增加了那些不是标准的拼音，但模糊输入中需要使用的拼音组合
PYTABLE         PYTable[] = {
    {"zuo", NULL}
    ,
    {"zun", NULL}
    ,
    {"zui", NULL}
    ,
    {"zuagn", &fc.bMisstype }
    ,
    {"zuang", &MHPY_C[5].bMode}
    ,
    {"zuagn", &fc.bMisstype }
    ,
    {"zuang", &MHPY_S[4].bMode}
    ,
    {"zuan", NULL}
    ,
    {"zua", &MHPY_S[4].bMode}
    ,
    {"zu", NULL}
    ,
    {"zou", NULL}
    ,
    {"zogn", &fc.bMisstype }
    ,
    {"zong", NULL}
    ,
    {"zi", NULL}
    ,
    {"zhuo", NULL}
    ,
    {"zhun", NULL}
    ,
    {"zhui", NULL}
    ,
    {"zhuagn", &fc.bMisstype }
    ,
    {"zhuang", NULL}
    ,
    {"zhuan", NULL}
    ,
    {"zhuai", NULL}
    ,
    {"zhua", NULL}
    ,
    {"zhu", NULL}
    ,
    {"zhou", NULL}
    ,
    {"zhogn", &fc.bMisstype }
    ,
    {"zhong", NULL}
    ,
    {"zhi", NULL}
    ,
    {"zhegn", &fc.bMisstype }
    ,
    {"zheng", NULL}
    ,
    {"zhen", NULL}
    ,
    {"zhe", NULL}
    ,
    {"zhao", NULL}
    ,
    {"zhagn", &fc.bMisstype }
    ,
    {"zhang", NULL}
    ,
    {"zhan", NULL}
    ,
    {"zhai", NULL}
    ,
    {"zha", NULL}
    ,
    {"zegn", &fc.bMisstype }
    ,
    {"zeng", NULL}
    ,
    {"zen", NULL}
    ,
    {"zei", NULL}
    ,
    {"ze", NULL}
    ,
    {"zao", NULL}
    ,
    {"zagn", &fc.bMisstype }
    ,
    {"zang", NULL}
    ,
    {"zan", NULL}
    ,
    {"zai", NULL}
    ,
    {"za", NULL}
    ,
    {"yun", NULL}
    ,
    {"yue", NULL}
    ,
    {"yuagn", &fc.bMisstype }
    ,
    {"yuang", &MHPY_C[5].bMode}
    ,
    {"yuan", NULL}
    ,
    {"yu", NULL}
    ,
    {"you", NULL}
    ,
    {"yogn", &fc.bMisstype }
    ,
    {"yong", NULL}
    ,
    {"yo", NULL}
    ,
    {"yign", &fc.bMisstype }
    ,
    {"ying", NULL}
    ,
    {"yin", NULL}
    ,
    {"yi", NULL}
    ,
    {"ye", NULL}
    ,
    {"yao", NULL}
    ,
    {"yagn", &fc.bMisstype }
    ,
    {"yang", NULL}
    ,
    {"yan", NULL}
    ,
    {"ya", NULL}
    ,
    {"xun", NULL}
    ,
    {"xue", NULL}
    ,
    {"xuagn", &fc.bMisstype }
    ,
    {"xuang", &MHPY_C[5].bMode}
    ,
    {"xuan", NULL}
    ,
    {"xu", NULL}
    ,
    {"xou", &MHPY_C[4].bMode}
    ,
    {"xiu", NULL}
    ,
    {"xiogn", &fc.bMisstype }
    ,
    {"xiong", NULL}
    ,
    {"xign", &fc.bMisstype }
    ,
    {"xing", NULL}
    ,
    {"xin", NULL}
    ,
    {"xie", NULL}
    ,
    {"xiao", NULL}
    ,
    {"xiagn", &fc.bMisstype }
    ,
    {"xiang", NULL}
    ,
    {"xian", NULL}
    ,
    {"xia", NULL}
    ,
    {"xi", NULL}
    ,
    {"wu", NULL}
    ,
    {"wo", NULL}
    ,
    {"wegn", &fc.bMisstype }
    ,
    {"weng", NULL}
    ,
    {"wen", NULL}
    ,
    {"wei", NULL}
    ,
    {"wagn", &fc.bMisstype }
    ,
    {"wang", NULL}
    ,
    {"wan", NULL}
    ,
    {"wai", NULL}
    ,
    {"wa", NULL}
    ,
    {"tuo", NULL}
    ,
    {"tun", NULL}
    ,
    {"tui", NULL}
    ,
    {"tuagn", &fc.bMisstype }
    ,
    {"tuang", &MHPY_C[5].bMode}
    ,
    {"tuan", NULL}
    ,
    {"tu", NULL}
    ,
    {"tou", NULL}
    ,
    {"togn", &fc.bMisstype }
    ,
    {"tong", NULL}
    ,
    {"tign", &fc.bMisstype }
    ,
    {"ting", NULL}
    ,
    {"tin", &MHPY_C[3].bMode}
    ,
    {"tie", NULL}
    ,
    {"tiao", NULL}
    ,
    {"tiagn", &fc.bMisstype }
    ,
    {"tiang", &MHPY_C[2].bMode}
    ,
    {"tian", NULL}
    ,
    {"ti", NULL}
    ,
    {"tegn", &fc.bMisstype }
    ,
    {"teng", NULL}
    ,
    {"ten", &MHPY_C[1].bMode}
    ,
    {"tei", NULL}
    ,
    {"te", NULL}
    ,
    {"tao", NULL}
    ,
    {"tagn", &fc.bMisstype }
    ,
    {"tang", NULL}
    ,
    {"tan", NULL}
    ,
    {"tai", NULL}
    ,
    {"ta", NULL}
    ,
    {"suo", NULL}
    ,
    {"sun", NULL}
    ,
    {"sui", NULL}
    ,
    {"suagn", &fc.bMisstype }
    ,
    {"suang", &MHPY_S[3].bMode}
    ,
    {"suagn", &fc.bMisstype }
    ,
    {"suang", &MHPY_C[5].bMode}
    ,
    {"suan", NULL}
    ,
    {"sua", &MHPY_S[3].bMode}
    ,
    {"su", NULL}
    ,
    {"sou", NULL}
    ,
    {"sogn", &fc.bMisstype }
    ,
    {"song", NULL}
    ,
    {"si", NULL}
    ,
    {"shuo", NULL}
    ,
    {"shun", NULL}
    ,
    {"shui", NULL}
    ,
    {"shuagn", &fc.bMisstype }
    ,
    {"shuang", NULL}
    ,
    {"shuan", NULL}
    ,
    {"shuai", NULL}
    ,
    {"shua", NULL}
    ,
    {"shu", NULL}
    ,
    {"shou", NULL}
    ,
    {"shi", NULL}
    ,
    {"shegn", &fc.bMisstype }
    ,
    {"sheng", NULL}
    ,
    {"shen", NULL}
    ,
    {"shei", NULL}
    ,
    {"she", NULL}
    ,
    {"shao", NULL}
    ,
    {"shagn", &fc.bMisstype }
    ,
    {"shang", NULL}
    ,
    {"shan", NULL}
    ,
    {"shai", NULL}
    ,
    {"sha", NULL}
    ,
    {"segn", &fc.bMisstype }
    ,
    {"seng", NULL}
    ,
    {"sen", NULL}
    ,
    {"se", NULL}
    ,
    {"sao", NULL}
    ,
    {"sagn", &fc.bMisstype }
    ,
    {"sang", NULL}
    ,
    {"san", NULL}
    ,
    {"sai", NULL}
    ,
    {"sa", NULL}
    ,
    {"ruo", NULL}
    ,
    {"run", NULL}
    ,
    {"rui", NULL}
    ,
    {"ruagn", &fc.bMisstype }
    ,
    {"ruang", &MHPY_C[5].bMode}
    ,
    {"ruan", NULL}
    ,
    {"ru", NULL}
    ,
    {"rou", NULL}
    ,
    {"rogn", &fc.bMisstype }
    ,
    {"rong", NULL}
    ,
    {"ri", NULL}
    ,
    {"regn", &fc.bMisstype }
    ,
    {"reng", NULL}
    ,
    {"ren", NULL}
    ,
    {"re", NULL}
    ,
    {"rao", NULL}
    ,
    {"ragn", &fc.bMisstype }
    ,
    {"rang", NULL}
    ,
    {"ran", NULL}
    ,
    {"qun", NULL}
    ,
    {"que", NULL}
    ,
    {"quagn", &fc.bMisstype }
    ,
    {"quang", &MHPY_C[5].bMode}
    ,
    {"quan", NULL}
    ,
    {"qu", NULL}
    ,
    {"qiu", NULL}
    ,
    {"qiogn", &fc.bMisstype }
    ,
    {"qiong", NULL}
    ,
    {"qign", &fc.bMisstype }
    ,
    {"qing", NULL}
    ,
    {"qin", NULL}
    ,
    {"qie", NULL}
    ,
    {"qiao", NULL}
    ,
    {"qiagn", &fc.bMisstype }
    ,
    {"qiang", NULL}
    ,
    {"qian", NULL}
    ,
    {"qia", NULL}
    ,
    {"qi", NULL}
    ,
    {"pu", NULL}
    ,
    {"pou", NULL}
    ,
    {"po", NULL}
    ,
    {"pign", &fc.bMisstype }
    ,
    {"ping", NULL}
    ,
    {"pin", NULL}
    ,
    {"pie", NULL}
    ,
    {"piao", NULL}
    ,
    {"piagn", &fc.bMisstype }
    ,
    {"piang", &MHPY_C[2].bMode}
    ,
    {"pian", NULL}
    ,
    {"pi", NULL}
    ,
    {"pegn", &fc.bMisstype }
    ,
    {"peng", NULL}
    ,
    {"pen", NULL}
    ,
    {"pei", NULL}
    ,
    {"pao", NULL}
    ,
    {"pagn", &fc.bMisstype }
    ,
    {"pang", NULL}
    ,
    {"pan", NULL}
    ,
    {"pai", NULL}
    ,
    {"pa", NULL}
    ,
    {"ou", NULL}
    ,
    {"o", NULL}
    ,
    {"nve", NULL}
    ,
    {"nv", NULL}
    ,
    {"nuo", NULL}
    ,
    {"nue", NULL}
    ,
    {"nuagn", &fc.bMisstype }
    ,
    {"nuang", &MHPY_C[5].bMode}
    ,
    {"nuagn", &fc.bMisstype }
    ,
    {"nuang", &MHPY_S[2].bMode}
    ,
    {"nuan", NULL}
    ,
    {"nu", NULL}
    ,
    {"nou", NULL}
    ,
    {"nogn", &fc.bMisstype }
    ,
    {"nong", NULL}
    ,
    {"niu", NULL}
    ,
    {"nign", &fc.bMisstype }
    ,
    {"ning", NULL}
    ,
    {"nin", NULL}
    ,
    {"nie", NULL}
    ,
    {"niao", NULL}
    ,
    {"niagn", &fc.bMisstype }
    ,
    {"niang", NULL}
    ,
    {"nian", NULL}
    ,
    {"ni", NULL}
    ,
    {"ng", NULL}
    ,
    {"negn", &fc.bMisstype }
    ,
    {"neng", NULL}
    ,
    {"nen", NULL}
    ,
    {"nei", NULL}
    ,
    {"ne", NULL}
    ,
    {"nao", NULL}
    ,
    {"nagn", &fc.bMisstype }
    ,
    {"nang", NULL}
    ,
    {"nan", NULL}
    ,
    {"nai", NULL}
    ,
    {"na", NULL}
    ,
    {"n", NULL}
    ,
    {"mu", NULL}
    ,
    {"mou", NULL}
    ,
    {"mo", NULL}
    ,
    {"miu", NULL}
    ,
    {"mign", &fc.bMisstype }
    ,
    {"ming", NULL}
    ,
    {"min", NULL}
    ,
    {"mie", NULL}
    ,
    {"miao", NULL}
    ,
    {"miagn", &fc.bMisstype }
    ,
    {"miang", &MHPY_C[2].bMode}
    ,
    {"mian", NULL}
    ,
    {"mi", NULL}
    ,
    {"megn", &fc.bMisstype }
    ,
    {"meng", NULL}
    ,
    {"men", NULL}
    ,
    {"mei", NULL}
    ,
    {"me", NULL}
    ,
    {"mao", NULL}
    ,
    {"magn", &fc.bMisstype }
    ,
    {"mang", NULL}
    ,
    {"man", NULL}
    ,
    {"mai", NULL}
    ,
    {"ma", NULL}
    ,
    {"m", NULL}
    ,
    {"lve", NULL}
    ,
    {"lv", NULL}
    ,
    {"luo", NULL}
    ,
    {"lun", NULL}
    ,
    {"lue", NULL}
    ,
    {"luagn", &fc.bMisstype }
    ,
    {"luang", &MHPY_C[5].bMode}
    ,
    {"luagn", &fc.bMisstype }
    ,
    {"luang", &MHPY_S[2].bMode}
    ,
    {"luan", NULL}
    ,
    {"lu", NULL}
    ,
    {"lou", NULL}
    ,
    {"logn", &fc.bMisstype }
    ,
    {"long", NULL}
    ,
    {"lo", NULL}
    ,
    {"liu", NULL}
    ,
    {"lign", &fc.bMisstype }
    ,
    {"ling", NULL}
    ,
    {"lin", NULL}
    ,
    {"lie", NULL}
    ,
    {"liao", NULL}
    ,
    {"liagn", &fc.bMisstype }
    ,
    {"liang", NULL}
    ,
    {"lian", NULL}
    ,
    {"lia", NULL}
    ,
    {"li", NULL}
    ,
    {"legn", &fc.bMisstype }
    ,
    {"leng", NULL}
    ,
    {"len", &MHPY_C[1].bMode}
    ,
    {"lei", NULL}
    ,
    {"le", NULL}
    ,
    {"lao", NULL}
    ,
    {"lagn", &fc.bMisstype }
    ,
    {"lang", NULL}
    ,
    {"lan", NULL}
    ,
    {"lai", NULL}
    ,
    {"la", NULL}
    ,
    {"kuo", NULL}
    ,
    {"kun", NULL}
    ,
    {"kui", NULL}
    ,
    {"kuagn", &fc.bMisstype }
    ,
    {"kuang", NULL}
    ,
    {"kuan", NULL}
    ,
    {"kuai", NULL}
    ,
    {"kua", NULL}
    ,
    {"ku", NULL}
    ,
    {"kou", NULL}
    ,
    {"kogn", &fc.bMisstype }
    ,
    {"kong", NULL}
    ,
    {"kegn", &fc.bMisstype }
    ,
    {"keng", NULL}
    ,
    {"ken", NULL}
    ,
    {"kei", NULL}
    ,
    {"ke", NULL}
    ,
    {"kao", NULL}
    ,
    {"kagn", &fc.bMisstype }
    ,
    {"kang", NULL}
    ,
    {"kan", NULL}
    ,
    {"kai", NULL}
    ,
    {"ka", NULL}
    ,
    {"jun", NULL}
    ,
    {"jue", NULL}
    ,
    {"juagn", &fc.bMisstype }
    ,
    {"juang", &MHPY_C[5].bMode}
    ,
    {"juan", NULL}
    ,
    {"ju", NULL}
    ,
    {"jiu", NULL}
    ,
    {"jiogn", &fc.bMisstype }
    ,
    {"jiong", NULL}
    ,
    {"jign", &fc.bMisstype }
    ,
    {"jing", NULL}
    ,
    {"jin", NULL}
    ,
    {"jie", NULL}
    ,
    {"jiao", NULL}
    ,
    {"jiagn", &fc.bMisstype }
    ,
    {"jiang", NULL}
    ,
    {"jian", NULL}
    ,
    {"jia", NULL}
    ,
    {"ji", NULL}
    ,
    {"huo", NULL}
    ,
    {"hun", NULL}
    ,
    {"hui", NULL}
    ,
    {"huagn", &fc.bMisstype }
    ,
    {"huang", NULL}
    ,
    {"huan", NULL}
    ,
    {"huai", NULL}
    ,
    {"hua", NULL}
    ,
    {"hu", NULL}
    ,
    {"hou", NULL}
    ,
    {"hogn", &fc.bMisstype }
    ,
    {"hong", NULL}
    ,
    {"hegn", &fc.bMisstype }
    ,
    {"heng", NULL}
    ,
    {"hen", NULL}
    ,
    {"hei", NULL}
    ,
    {"he", NULL}
    ,
    {"hao", NULL}
    ,
    {"hagn", &fc.bMisstype }
    ,
    {"hang", NULL}
    ,
    {"han", NULL}
    ,
    {"hai", NULL}
    ,
    {"ha", NULL}
    ,
    {"guo", NULL}
    ,
    {"gun", NULL}
    ,
    {"gui", NULL}
    ,
    {"guagn", &fc.bMisstype }
    ,
    {"guang", NULL}
    ,
    {"guan", NULL}
    ,
    {"guai", NULL}
    ,
    {"gua", NULL}
    ,
    {"gu", NULL}
    ,
    {"gou", NULL}
    ,
    {"gogn", &fc.bMisstype }
    ,
    {"gong", NULL}
    ,
    {"gegn", &fc.bMisstype }
    ,
    {"geng", NULL}
    ,
    {"gen", NULL}
    ,
    {"gei", NULL}
    ,
    {"ge", NULL}
    ,
    {"gao", NULL}
    ,
    {"gagn", &fc.bMisstype }
    ,
    {"gang", NULL}
    ,
    {"gan", NULL}
    ,
    {"gai", NULL}
    ,
    {"ga", NULL}
    ,
    {"fu", NULL}
    ,
    {"fou", NULL}
    ,
    {"fo", NULL}
    ,
    {"fegn", &fc.bMisstype }
    ,
    {"feng", NULL}
    ,
    {"fen", NULL}
    ,
    {"fei", NULL}
    ,
    {"fagn", &fc.bMisstype }
    ,
    {"fang", NULL}
    ,
    {"fan", NULL}
    ,
    {"fa", NULL}
    ,
    {"er", NULL}
    ,
    {"egn", &fc.bMisstype }
    ,
    {"eng", &MHPY_C[1].bMode}
    ,
    {"en", NULL}
    ,
    {"ei", NULL}
    ,
    {"e", NULL}
    ,
    {"duo", NULL}
    ,
    {"dun", NULL}
    ,
    {"dui", NULL}
    ,
    {"duagn", &fc.bMisstype }
    ,
    {"duang", &MHPY_C[5].bMode}
    ,
    {"duan", NULL}
    ,
    {"du", NULL}
    ,
    {"dou", NULL}
    ,
    {"dogn", &fc.bMisstype }
    ,
    {"dong", NULL}
    ,
    {"diu", NULL}
    ,
    {"dign", &fc.bMisstype }
    ,
    {"ding", NULL}
    ,
    {"din", &MHPY_C[3].bMode}
    ,
    {"die", NULL}
    ,
    {"diao", NULL}
    ,
    {"diagn", &fc.bMisstype }
    ,
    {"diang", &MHPY_C[2].bMode}
    ,
    {"dian", NULL}
    ,
    {"dia", NULL}
    ,
    {"di", NULL}
    ,
    {"degn", &fc.bMisstype }
    ,
    {"deng", NULL}
    ,
    {"den", NULL}
    ,
    {"dei", NULL}
    ,
    {"de", NULL}
    ,
    {"dao", NULL}
    ,
    {"dagn", &fc.bMisstype }
    ,
    {"dang", NULL}
    ,
    {"dan", NULL}
    ,
    {"dai", NULL}
    ,
    {"da", NULL}
    ,
    {"cuo", NULL}
    ,
    {"cun", NULL}
    ,
    {"cui", NULL}
    ,
    {"cuagn", &fc.bMisstype }
    ,
    {"cuang", &MHPY_C[5].bMode}
    ,
    {"cuagn", &fc.bMisstype }
    ,
    {"cuang", &MHPY_S[0].bMode}
    ,
    {"cuan", NULL}
    ,
    {"cu", NULL}
    ,
    {"cou", NULL}
    ,
    {"cogn", &fc.bMisstype }
    ,
    {"cong", NULL}
    ,
    {"ci", NULL}
    ,
    {"chuo", NULL}
    ,
    {"chun", NULL}
    ,
    {"chui", NULL}
    ,
    {"chuagn", &fc.bMisstype }
    ,
    {"chuang", NULL}
    ,
    {"chuan", NULL}
    ,
    {"chuai", NULL}
    ,
    {"chu", NULL}
    ,
    {"chou", NULL}
    ,
    {"chogn", &fc.bMisstype }
    ,
    {"chong", NULL}
    ,
    {"chi", NULL}
    ,
    {"chegn", &fc.bMisstype }
    ,
    {"cheng", NULL}
    ,
    {"chen", NULL}
    ,
    {"che", NULL}
    ,
    {"chao", NULL}
    ,
    {"chagn", &fc.bMisstype }
    ,
    {"chang", NULL}
    ,
    {"chan", NULL}
    ,
    {"chai", NULL}
    ,
    {"cha", NULL}
    ,
    {"cegn", &fc.bMisstype }
    ,
    {"ceng", NULL}
    ,
    {"cen", NULL}
    ,
    {"ce", NULL}
    ,
    {"cao", NULL}
    ,
    {"cagn", &fc.bMisstype }
    ,
    {"cang", NULL}
    ,
    {"can", NULL}
    ,
    {"cai", NULL}
    ,
    {"ca", NULL}
    ,
    {"bu", NULL}
    ,
    {"bo", NULL}
    ,
    {"bign", &fc.bMisstype }
    ,
    {"bing", NULL}
    ,
    {"bin", NULL}
    ,
    {"bie", NULL}
    ,
    {"biao", NULL}
    ,
    {"biagn", &fc.bMisstype }
    ,
    {"biang", &MHPY_C[2].bMode}
    ,
    {"bian", NULL}
    ,
    {"bi", NULL}
    ,
    {"begn", &fc.bMisstype }
    ,
    {"beng", NULL}
    ,
    {"ben", NULL}
    ,
    {"bei", NULL}
    ,
    {"bao", NULL}
    ,
    {"bagn", &fc.bMisstype }
    ,
    {"bang", NULL}
    ,
    {"ban", NULL}
    ,
    {"bai", NULL}
    ,
    {"ba", NULL}
    ,
    {"ao", NULL}
    ,
    {"agn", &fc.bMisstype }
    ,
    {"ang", NULL}
    ,
    {"an", NULL}
    ,
    {"ai", NULL}
    ,
    {"a", NULL}
    ,
    {"\0", NULL}
};

int GetMHIndex_C (char map)
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

int GetMHIndex_S (char map, Bool bMode)
{
    int             i;

    for (i = 0; MHPY_S[i].strMap[0]; i++) {
        if (map == MHPY_S[i].strMap[0] || map == MHPY_S[i].strMap[1]) {
            if (MHPY_S[i].bMode || bMode)
                return i;
            else
                return -1;
        }
    }

    return -1;

}

Bool IsZ_C_S (char map)
{
    if (map=='c' || map=='H'|| map=='B')
        return True;
    return False;
}

