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
#include "PYFA.h"
#include "pyconfig.h"

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
    {"zuagn", &pyconfig.bMisstype }
    ,
    {"zuang", &MHPY_C[5].bMode}
    ,
    {"zuagn", &pyconfig.bMisstype }
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
    {"zogn", &pyconfig.bMisstype }
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
    {"zhuagn", &pyconfig.bMisstype }
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
    {"zhogn", &pyconfig.bMisstype }
    ,
    {"zhong", NULL}
    ,
    {"zhi", NULL}
    ,
    {"zhegn", &pyconfig.bMisstype }
    ,
    {"zheng", NULL}
    ,
    {"zhen", NULL}
    ,
    {"zhe", NULL}
    ,
    {"zhao", NULL}
    ,
    {"zhagn", &pyconfig.bMisstype }
    ,
    {"zhang", NULL}
    ,
    {"zhan", NULL}
    ,
    {"zhai", NULL}
    ,
    {"zha", NULL}
    ,
    {"zegn", &pyconfig.bMisstype }
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
    {"zagn", &pyconfig.bMisstype }
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
    {"yuagn", &pyconfig.bMisstype }
    ,
    {"yuang", &MHPY_C[5].bMode}
    ,
    {"yuan", NULL}
    ,
    {"yu", NULL}
    ,
    {"you", NULL}
    ,
    {"yogn", &pyconfig.bMisstype }
    ,
    {"yong", NULL}
    ,
    {"yo", NULL}
    ,
    {"yign", &pyconfig.bMisstype }
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
    {"yagn", &pyconfig.bMisstype }
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
    {"xuagn", &pyconfig.bMisstype }
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
    {"xiogn", &pyconfig.bMisstype }
    ,
    {"xiong", NULL}
    ,
    {"xign", &pyconfig.bMisstype }
    ,
    {"xing", NULL}
    ,
    {"xin", NULL}
    ,
    {"xie", NULL}
    ,
    {"xiao", NULL}
    ,
    {"xiagn", &pyconfig.bMisstype }
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
    {"wegn", &pyconfig.bMisstype }
    ,
    {"weng", NULL}
    ,
    {"wen", NULL}
    ,
    {"wei", NULL}
    ,
    {"wagn", &pyconfig.bMisstype }
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
    {"tuagn", &pyconfig.bMisstype }
    ,
    {"tuang", &MHPY_C[5].bMode}
    ,
    {"tuan", NULL}
    ,
    {"tu", NULL}
    ,
    {"tou", NULL}
    ,
    {"togn", &pyconfig.bMisstype }
    ,
    {"tong", NULL}
    ,
    {"tign", &pyconfig.bMisstype }
    ,
    {"ting", NULL}
    ,
    {"tin", &MHPY_C[3].bMode}
    ,
    {"tie", NULL}
    ,
    {"tiao", NULL}
    ,
    {"tiagn", &pyconfig.bMisstype }
    ,
    {"tiang", &MHPY_C[2].bMode}
    ,
    {"tian", NULL}
    ,
    {"ti", NULL}
    ,
    {"tegn", &pyconfig.bMisstype }
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
    {"tagn", &pyconfig.bMisstype }
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
    {"suagn", &pyconfig.bMisstype }
    ,
    {"suang", &MHPY_S[3].bMode}
    ,
    {"suagn", &pyconfig.bMisstype }
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
    {"sogn", &pyconfig.bMisstype }
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
    {"shuagn", &pyconfig.bMisstype }
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
    {"shegn", &pyconfig.bMisstype }
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
    {"shagn", &pyconfig.bMisstype }
    ,
    {"shang", NULL}
    ,
    {"shan", NULL}
    ,
    {"shai", NULL}
    ,
    {"sha", NULL}
    ,
    {"segn", &pyconfig.bMisstype }
    ,
    {"seng", NULL}
    ,
    {"sen", NULL}
    ,
    {"se", NULL}
    ,
    {"sao", NULL}
    ,
    {"sagn", &pyconfig.bMisstype }
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
    {"ruagn", &pyconfig.bMisstype }
    ,
    {"ruang", &MHPY_C[5].bMode}
    ,
    {"ruan", NULL}
    ,
    {"ru", NULL}
    ,
    {"rou", NULL}
    ,
    {"rogn", &pyconfig.bMisstype }
    ,
    {"rong", NULL}
    ,
    {"ri", NULL}
    ,
    {"regn", &pyconfig.bMisstype }
    ,
    {"reng", NULL}
    ,
    {"ren", NULL}
    ,
    {"re", NULL}
    ,
    {"rao", NULL}
    ,
    {"ragn", &pyconfig.bMisstype }
    ,
    {"rang", NULL}
    ,
    {"ran", NULL}
    ,
    {"qun", NULL}
    ,
    {"que", NULL}
    ,
    {"quagn", &pyconfig.bMisstype }
    ,
    {"quang", &MHPY_C[5].bMode}
    ,
    {"quan", NULL}
    ,
    {"qu", NULL}
    ,
    {"qiu", NULL}
    ,
    {"qiogn", &pyconfig.bMisstype }
    ,
    {"qiong", NULL}
    ,
    {"qign", &pyconfig.bMisstype }
    ,
    {"qing", NULL}
    ,
    {"qin", NULL}
    ,
    {"qie", NULL}
    ,
    {"qiao", NULL}
    ,
    {"qiagn", &pyconfig.bMisstype }
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
    {"pign", &pyconfig.bMisstype }
    ,
    {"ping", NULL}
    ,
    {"pin", NULL}
    ,
    {"pie", NULL}
    ,
    {"piao", NULL}
    ,
    {"piagn", &pyconfig.bMisstype }
    ,
    {"piang", &MHPY_C[2].bMode}
    ,
    {"pian", NULL}
    ,
    {"pi", NULL}
    ,
    {"pegn", &pyconfig.bMisstype }
    ,
    {"peng", NULL}
    ,
    {"pen", NULL}
    ,
    {"pei", NULL}
    ,
    {"pao", NULL}
    ,
    {"pagn", &pyconfig.bMisstype }
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
    {"nuagn", &pyconfig.bMisstype }
    ,
    {"nuang", &MHPY_C[5].bMode}
    ,
    {"nuagn", &pyconfig.bMisstype }
    ,
    {"nuang", &MHPY_S[2].bMode}
    ,
    {"nuan", NULL}
    ,
    {"nu", NULL}
    ,
    {"nou", NULL}
    ,
    {"nogn", &pyconfig.bMisstype }
    ,
    {"nong", NULL}
    ,
    {"niu", NULL}
    ,
    {"nign", &pyconfig.bMisstype }
    ,
    {"ning", NULL}
    ,
    {"nin", NULL}
    ,
    {"nie", NULL}
    ,
    {"niao", NULL}
    ,
    {"niagn", &pyconfig.bMisstype }
    ,
    {"niang", NULL}
    ,
    {"nian", NULL}
    ,
    {"ni", NULL}
    ,
    {"ng", NULL}
    ,
    {"negn", &pyconfig.bMisstype }
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
    {"nagn", &pyconfig.bMisstype }
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
    {"mign", &pyconfig.bMisstype }
    ,
    {"ming", NULL}
    ,
    {"min", NULL}
    ,
    {"mie", NULL}
    ,
    {"miao", NULL}
    ,
    {"miagn", &pyconfig.bMisstype }
    ,
    {"miang", &MHPY_C[2].bMode}
    ,
    {"mian", NULL}
    ,
    {"mi", NULL}
    ,
    {"megn", &pyconfig.bMisstype }
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
    {"magn", &pyconfig.bMisstype }
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
    {"luagn", &pyconfig.bMisstype }
    ,
    {"luang", &MHPY_C[5].bMode}
    ,
    {"luagn", &pyconfig.bMisstype }
    ,
    {"luang", &MHPY_S[2].bMode}
    ,
    {"luan", NULL}
    ,
    {"lu", NULL}
    ,
    {"lou", NULL}
    ,
    {"logn", &pyconfig.bMisstype }
    ,
    {"long", NULL}
    ,
    {"lo", NULL}
    ,
    {"liu", NULL}
    ,
    {"lign", &pyconfig.bMisstype }
    ,
    {"ling", NULL}
    ,
    {"lin", NULL}
    ,
    {"lie", NULL}
    ,
    {"liao", NULL}
    ,
    {"liagn", &pyconfig.bMisstype }
    ,
    {"liang", NULL}
    ,
    {"lian", NULL}
    ,
    {"lia", NULL}
    ,
    {"li", NULL}
    ,
    {"legn", &pyconfig.bMisstype }
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
    {"lagn", &pyconfig.bMisstype }
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
    {"kuagn", &pyconfig.bMisstype }
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
    {"kogn", &pyconfig.bMisstype }
    ,
    {"kong", NULL}
    ,
    {"kegn", &pyconfig.bMisstype }
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
    {"kagn", &pyconfig.bMisstype }
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
    {"juagn", &pyconfig.bMisstype }
    ,
    {"juang", &MHPY_C[5].bMode}
    ,
    {"juan", NULL}
    ,
    {"ju", NULL}
    ,
    {"jiu", NULL}
    ,
    {"jiogn", &pyconfig.bMisstype }
    ,
    {"jiong", NULL}
    ,
    {"jign", &pyconfig.bMisstype }
    ,
    {"jing", NULL}
    ,
    {"jin", NULL}
    ,
    {"jie", NULL}
    ,
    {"jiao", NULL}
    ,
    {"jiagn", &pyconfig.bMisstype }
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
    {"huagn", &pyconfig.bMisstype }
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
    {"hogn", &pyconfig.bMisstype }
    ,
    {"hong", NULL}
    ,
    {"hegn", &pyconfig.bMisstype }
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
    {"hagn", &pyconfig.bMisstype }
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
    {"guagn", &pyconfig.bMisstype }
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
    {"gogn", &pyconfig.bMisstype }
    ,
    {"gong", NULL}
    ,
    {"gegn", &pyconfig.bMisstype }
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
    {"gagn", &pyconfig.bMisstype }
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
    {"fegn", &pyconfig.bMisstype }
    ,
    {"feng", NULL}
    ,
    {"fen", NULL}
    ,
    {"fei", NULL}
    ,
    {"fagn", &pyconfig.bMisstype }
    ,
    {"fang", NULL}
    ,
    {"fan", NULL}
    ,
    {"fa", NULL}
    ,
    {"er", NULL}
    ,
    {"egn", &pyconfig.bMisstype }
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
    {"duagn", &pyconfig.bMisstype }
    ,
    {"duang", &MHPY_C[5].bMode}
    ,
    {"duan", NULL}
    ,
    {"du", NULL}
    ,
    {"dou", NULL}
    ,
    {"dogn", &pyconfig.bMisstype }
    ,
    {"dong", NULL}
    ,
    {"diu", NULL}
    ,
    {"dign", &pyconfig.bMisstype }
    ,
    {"ding", NULL}
    ,
    {"din", &MHPY_C[3].bMode}
    ,
    {"die", NULL}
    ,
    {"diao", NULL}
    ,
    {"diagn", &pyconfig.bMisstype }
    ,
    {"diang", &MHPY_C[2].bMode}
    ,
    {"dian", NULL}
    ,
    {"dia", NULL}
    ,
    {"di", NULL}
    ,
    {"degn", &pyconfig.bMisstype }
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
    {"dagn", &pyconfig.bMisstype }
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
    {"cuagn", &pyconfig.bMisstype }
    ,
    {"cuang", &MHPY_C[5].bMode}
    ,
    {"cuagn", &pyconfig.bMisstype }
    ,
    {"cuang", &MHPY_S[0].bMode}
    ,
    {"cuan", NULL}
    ,
    {"cu", NULL}
    ,
    {"cou", NULL}
    ,
    {"cogn", &pyconfig.bMisstype }
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
    {"chuagn", &pyconfig.bMisstype }
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
    {"chogn", &pyconfig.bMisstype }
    ,
    {"chong", NULL}
    ,
    {"chi", NULL}
    ,
    {"chegn", &pyconfig.bMisstype }
    ,
    {"cheng", NULL}
    ,
    {"chen", NULL}
    ,
    {"che", NULL}
    ,
    {"chao", NULL}
    ,
    {"chagn", &pyconfig.bMisstype }
    ,
    {"chang", NULL}
    ,
    {"chan", NULL}
    ,
    {"chai", NULL}
    ,
    {"cha", NULL}
    ,
    {"cegn", &pyconfig.bMisstype }
    ,
    {"ceng", NULL}
    ,
    {"cen", NULL}
    ,
    {"ce", NULL}
    ,
    {"cao", NULL}
    ,
    {"cagn", &pyconfig.bMisstype }
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
    {"bign", &pyconfig.bMisstype }
    ,
    {"bing", NULL}
    ,
    {"bin", NULL}
    ,
    {"bie", NULL}
    ,
    {"biao", NULL}
    ,
    {"biagn", &pyconfig.bMisstype }
    ,
    {"biang", &MHPY_C[2].bMode}
    ,
    {"bian", NULL}
    ,
    {"bi", NULL}
    ,
    {"begn", &pyconfig.bMisstype }
    ,
    {"beng", NULL}
    ,
    {"ben", NULL}
    ,
    {"bei", NULL}
    ,
    {"bao", NULL}
    ,
    {"bagn", &pyconfig.bMisstype }
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
    {"agn", &pyconfig.bMisstype }
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

