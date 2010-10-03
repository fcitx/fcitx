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
#include <stdio.h>
#include <limits.h>
#include <ctype.h>

#include "tools/tools.h"
#include "im/special/QuickPhrase.h"
#include "ui/InputWindow.h"
#include "tools/utarray.h"
#include "fcitx-config/cutils.h"
#include "fcitx-config/xdg.h"
#include "fcitx-config/configfile.h"

uint uQuickPhraseCount;
UT_array *quickPhrases = NULL;
int iFirstQuickPhrase = -1;
int iLastQuickPhrase;
QUICK_PHRASE *quickPhraseCandWords[MAX_CAND_WORD];
static UT_icd qp_icd = {sizeof(QUICK_PHRASE), NULL, NULL, NULL};

#define MIN(a,b) ((a) < (b)?(a) : (b))

extern int iCandPageCount;
extern int iCandWordCount;
extern int iCurrentCandPage;
extern char strCodeInput[];
extern char strStringGet[];

int PhraseCmp(const void* a, const void* b)
{
    return strcmp(((QUICK_PHRASE*)a)->strCode, ((QUICK_PHRASE*)b)->strCode);
}

int PhraseCmpA(const void* a, const void* b)
{
    int res,len;
    len = strlen(((QUICK_PHRASE*)a)->strCode);
    res = strncmp(((QUICK_PHRASE*)a)->strCode, ((QUICK_PHRASE*)b)->strCode, len);
    if (res)
        return res;
    else
    {
        return 1;
    }

}

/**
 * @brief 加载快速输入词典
 * @param void
 * @return void
 * @note 快速输入是在;的行为定义为2,并且输入;以后才可以使用的。
 * 加载快速输入词典.如：输入“zg”就直接出现“中华人民共和国”等等。
 * 文件中每一行数据的定义为：<字符组合> <短语>
 * 如：“zg 中华人民共和国”
 */
void LoadQuickPhrase(void)
{
    FILE *fp;
    char *buf = NULL;
    size_t len = 0;
    char *buf1 = NULL;

    QUICK_PHRASE tempQuickPhrase;

    uQuickPhraseCount=0;

    fp =  GetXDGFileData("QuickPhrase.mb", "rt", NULL);
    if (!fp)
        return;

    // 这儿的处理比较简单。因为是单索引对单值。
    // 应该注意的是，它在内存中是以单链表保存的。
    utarray_new(quickPhrases, &qp_icd);
    while(getline(&buf, &len, fp) != -1)
    {
        if (buf1)
            free(buf1);
        buf1 = trim(buf);
        char *p = buf1;

        while(*p && !isspace(*p))
            p ++;

        if (p == '\0')
            continue;

        while (isspace(*p))
        {
            *p = '\0';
            p ++;
        }

        if (strlen(buf1) >= QUICKPHRASE_CODE_LEN)
            continue;

        if (strlen(p) >= QUICKPHRASE_PHRASE_LEN * UTF8_MAX_LENGTH + 1)
            continue;

        strcpy(tempQuickPhrase.strCode,buf1);
        strcpy(tempQuickPhrase.strPhrase, p);
        
        utarray_push_back(quickPhrases, &tempQuickPhrase);
    }

    if (buf)
        free(buf);
    if (buf1)
        free(buf1);

    if (quickPhrases)
    {
        utarray_sort(quickPhrases, PhraseCmp);
    }

    fclose(fp);
}

void FreeQuickPhrase(void)
{
    if ( !quickPhrases )
    return;

    utarray_free(quickPhrases);
}

INPUT_RETURN_VALUE QuickPhraseDoInput (int iKey)
{
    int retVal;

    if (iKey >= '0' && iKey <= '9') {
    if (!iCandWordCount)
        return IRV_TO_PROCESS;

    iKey -= '0';
    if (iKey == 0)
        iKey = 10;

    if (iKey > iCandWordCount)
        retVal = IRV_DO_NOTHING;
    else {
        strcpy (strStringGet, quickPhraseCandWords[iKey-1]->strPhrase);
        retVal = IRV_GET_CANDWORDS;
        SetMessageCount(&messageDown, 0);
    }
    }
    else if (iKey==' ') {
    if (!iCandWordCount)
        retVal = IRV_TO_PROCESS;
    else {
        strcpy (strStringGet, quickPhraseCandWords[0]->strPhrase);
        retVal = IRV_GET_CANDWORDS;
        SetMessageCount(&messageDown, 0);
    }
    }
    else
    retVal = IRV_TO_PROCESS;

    return retVal;
}

INPUT_RETURN_VALUE QuickPhraseGetCandWords (SEARCH_MODE mode)
{
    int i, iInputLen;
    QUICK_PHRASE searchKey, *pKey, *currentQuickPhrase, *lastQuickPhrase;
    char strTemp[2];

    pKey = &searchKey;

    if ( !quickPhrases )
        return IRV_DISPLAY_MESSAGE;

    iInputLen = strlen(strCodeInput);
    if (iInputLen > QUICKPHRASE_CODE_LEN)
        return IRV_DISPLAY_MESSAGE;

    strcpy(searchKey.strCode, strCodeInput);

    if (mode==SM_FIRST) {
        iCandPageCount=0;
        iCurrentCandPage=0;
        iCandWordCount=0;
        currentQuickPhrase = utarray_custom_bsearch(pKey, quickPhrases, False, PhraseCmp);
        iFirstQuickPhrase = utarray_eltidx(quickPhrases, currentQuickPhrase);
        lastQuickPhrase = utarray_custom_bsearch(pKey, quickPhrases, False, PhraseCmpA);
        iLastQuickPhrase = utarray_eltidx(quickPhrases, lastQuickPhrase);
        iCandPageCount = (iLastQuickPhrase - iFirstQuickPhrase + fc.iMaxCandWord - 1) / fc.iMaxCandWord - 1;
        if ( !currentQuickPhrase || strncmp(strCodeInput,currentQuickPhrase->strCode,iInputLen) ) {
            SetMessageCount(&messageDown, 0);
            currentQuickPhrase = NULL;
            return IRV_DISPLAY_MESSAGE;
        }
    }
    else if (mode==SM_NEXT) {
        if (iCurrentCandPage >= iCandPageCount)
            return IRV_DO_NOTHING;
        iCandWordCount=0;
        iCurrentCandPage++;
    }
    else {
        if (iCurrentCandPage <= 0)
            return IRV_DO_NOTHING;
        iCandWordCount=0;
        iCurrentCandPage--;
    }

    for( currentQuickPhrase = (QUICK_PHRASE*) utarray_eltptr(quickPhrases, iFirstQuickPhrase + iCurrentCandPage * fc.iMaxCandWord);
         currentQuickPhrase != NULL;
         currentQuickPhrase = (QUICK_PHRASE*) utarray_next(quickPhrases, currentQuickPhrase))
    {
        if (!strncmp(strCodeInput,currentQuickPhrase->strCode,iInputLen)) {
            quickPhraseCandWords[iCandWordCount++]=currentQuickPhrase;
            if (iCandWordCount==fc.iMaxCandWord) {
                break;
            }
        }
    }

    if (!iCandWordCount)
        return IRV_DISPLAY_MESSAGE;

    SetMessageCount(&messageDown, 0);
    strTemp[1]='\0';

    for (i = 0; i < iCandWordCount; i++) {
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';
        AddMessageAtLast(&messageDown, MSG_INDEX, strTemp);
        AddMessageAtLast(&messageDown, ((i == 0) ? MSG_FIRSTCAND : MSG_OTHER), quickPhraseCandWords[i]->strPhrase);

        //编码提示
        AddMessageAtLast(&messageDown, MSG_CODE, quickPhraseCandWords[i]->strCode + iInputLen);
        if (i != (iCandWordCount - 1))
            MessageConcatLast(&messageDown, " ");
    }

    return IRV_DISPLAY_CANDWORDS;
}
