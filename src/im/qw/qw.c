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
/*
 * 区位的算法来自于rfinput-2.0
 */
#include <string.h>
#include <iconv.h>

#include "core/fcitx.h"
#include "utils/utils.h"
#include "core/keys.h"

#include "qw.h"
#include "core/ui.h"
#include "utils/configfile.h"
#include "core/ime-internal.h"

extern FcitxInputState input;

INPUT_RETURN_VALUE DoQWInput(unsigned int sym, unsigned int state, int keyCount);
INPUT_RETURN_VALUE QWGetCandWords (SEARCH_MODE mode);
char *QWGetCandWord (int iIndex);
char           *GetQuWei (int, int);
boolean QWInit();

char     strQWHZ[3];
char     strQWHZUTF8[UTF8_MAX_LENGTH + 1];

FCITX_EXPORT_API
FcitxIM ime = {
    "Quwei",
    "qw",
    NULL,
    DoQWInput,
    QWGetCandWords,
    QWGetCandWord,
    NULL,
    NULL,
    QWInit,
    NULL,
    NULL
};

boolean QWInit()
{
    return true;
}

INPUT_RETURN_VALUE DoQWInput(unsigned int sym, unsigned int state, int keyCount)
{
    FcitxState* gs = GetFcitxGlobalState();
    INPUT_RETURN_VALUE retVal;

    retVal = IRV_TO_PROCESS;
    if (IsHotKeyDigit(sym, state)) {
        if ( input.iCodeInputCount!= 4 ) {
            input.strCodeInput[input.iCodeInputCount++]=sym;
            input.strCodeInput[input.iCodeInputCount]='\0';
            if ( input.iCodeInputCount==4 ) {
                strcpy(input.strStringGet, QWGetCandWord(sym-'0'-1));
                retVal= IRV_GET_CANDWORDS;
            }
            else if (input.iCodeInputCount==3)
                retVal=QWGetCandWords(SM_FIRST);
            else
                retVal=IRV_DISPLAY_CANDWORDS;
        }
    }
    else if (IsHotKey(sym, state, FCITX_BACKSPACE)) {
        if (!input.iCodeInputCount)
            return IRV_DONOT_PROCESS_CLEAN;
        input.iCodeInputCount--;
        input.strCodeInput[input.iCodeInputCount] = '\0';

        if (!input.iCodeInputCount)
            retVal = IRV_CLEAN;
        else {
            input.iCandPageCount = 0;
            SetMessageCount(gs->messageDown, 0);
            retVal = IRV_DISPLAY_CANDWORDS;
        }
    }
    else if (IsHotKey(sym, state, FCITX_SPACE)) {
        if (!input.iCodeInputCount)
            return IRV_TO_PROCESS;
        if (input.iCodeInputCount!=3)
            return IRV_DO_NOTHING;

        strcpy(input.strStringGet, QWGetCandWord(0));
        retVal= IRV_GET_CANDWORDS;
    }
    else
        return IRV_TO_PROCESS;

    SetMessageCount(gs->messageUp, 0);
    AddMessageAtLast(gs->messageUp, MSG_INPUT, "%s", input.strCodeInput);
    if ( input.iCodeInputCount!=3 )
        SetMessageCount(gs->messageDown, 0);

    return retVal;
}

char *QWGetCandWord (int iIndex)
{
    if ( !input.iCandPageCount )
        return NULL;

    FcitxState* gs = GetFcitxGlobalState();
    SetMessageCount(gs->messageDown, 0);
    if ( iIndex==-1 )
        iIndex=9;
    return GetQuWei((input.strCodeInput[0] - '0') * 10 + input.strCodeInput[1] - '0',input.iCurrentCandPage * 10+iIndex+1);
}

INPUT_RETURN_VALUE QWGetCandWords (SEARCH_MODE mode)
{
    int             iQu, iWei;
    int             i;
    char            strTemp[3];

    if ( ConfigGetPointAfterNumber() ) {
        strTemp[1] = '.';
        strTemp[2] = '\0';
    }
    else
        strTemp[1]='\0';

    iQu = (input.strCodeInput[0] - '0') * 10 + input.strCodeInput[1] - '0';

    if (mode==SM_FIRST ) {
        input.iCandPageCount = 9;
        input.iCurrentCandPage = input.strCodeInput[2]-'0';
    }
    else {
        if ( !input.iCandPageCount )
            return IRV_TO_PROCESS;
        if (mode==SM_NEXT) {
            if ( input.iCurrentCandPage!=input.iCandPageCount )
                input.iCurrentCandPage++;
        }
        else {
            if ( input.iCurrentCandPage )
                input.iCurrentCandPage--;
        }
    }

    iWei = input.iCurrentCandPage * 10;

    FcitxState* gs = GetFcitxGlobalState();
    SetMessageCount(gs->messageDown, 0);
    for (i = 0; i < 10; i++) {
        strTemp[0] = i + 1 + '0';
        if (i == 9)
            strTemp[0] = '0';
        AddMessageAtLast(gs->messageDown, MSG_INDEX, "%s", strTemp);
        AddMessageAtLast(gs->messageDown, (i)? MSG_OTHER:MSG_FIRSTCAND, "%s", GetQuWei (iQu, iWei + i + 1));
        if (i != 9)
            MessageConcatLast(gs->messageDown, " ");
    }

    input.strCodeInput[2]=input.iCurrentCandPage+'0';
    SetMessageCount(gs->messageUp, 0);
    AddMessageAtLast(gs->messageUp, MSG_INPUT, "%s", input.strCodeInput);

    return IRV_DISPLAY_CANDWORDS;
}

char           *GetQuWei (int iQu, int iWei)
{

    char *inbuf, *outbuf;

    size_t insize = 2, avail = UTF8_MAX_LENGTH + 1;

    iconv_t convGBK = iconv_open("utf-8", "gb18030");

    if (iQu >= 95) {      /* Process extend Qu 95 and 96 */
        strQWHZ[0] = iQu - 95 + 0xA8;
        strQWHZ[1] = iWei + 0x40;

        /* skip 0xa87f and 0xa97f */
        if ((unsigned char) strQWHZ[1] >= 0x7f)
            strQWHZ[1]++;
    }
    else {
        strQWHZ[0] = iQu + 0xa0;
        strQWHZ[1] = iWei + 0xa0;
    }

    strQWHZ[2] = '\0';

    inbuf = strQWHZ;

    outbuf = strQWHZUTF8;

    iconv(convGBK, &inbuf, &insize, &outbuf, &avail);

    iconv_close(convGBK);

    return strQWHZUTF8;
}