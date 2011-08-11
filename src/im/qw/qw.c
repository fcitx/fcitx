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
#include <libintl.h>

#include "fcitx/fcitx.h"
#include "fcitx-utils/utils.h"
#include "fcitx/keys.h"

#include "fcitx/ui.h"
#include "fcitx/configfile.h"
#include "fcitx/instance.h"
#include "fcitx/candidate.h"

typedef struct _FcitxQWState {
    char     strQWHZ[3];
    char     strQWHZUTF8[UTF8_MAX_LENGTH + 1];
    FcitxInstance *owner;
} FcitxQWState;

static void* QWCreate (struct _FcitxInstance* instance);
INPUT_RETURN_VALUE DoQWInput(void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE QWGetCandWords (void *arg);
INPUT_RETURN_VALUE QWGetCandWord (void *arg, CandidateWord* candWord);
char           *GetQuWei (FcitxQWState* qwstate, int iQu, int iWei);
boolean QWInit(void *arg);

FCITX_EXPORT_API
FcitxIMClass ime = {
    QWCreate,
    NULL
};

void* QWCreate (struct _FcitxInstance* instance)
{
    FcitxQWState* qwstate = fcitx_malloc0(sizeof(FcitxQWState));
    FcitxRegisterIM(
        instance,
        qwstate,
        _("Quwei"),
        "quwei",
        QWInit,
        NULL,
        DoQWInput,
        QWGetCandWords,
        NULL,
        NULL,
        NULL,
        NULL,
        100 /* make quwei place at last */
    );
    qwstate->owner = instance;
    return qwstate;
}

boolean QWInit(void *arg)
{
    return true;
}

INPUT_RETURN_VALUE DoQWInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = &qwstate->owner->input;
    INPUT_RETURN_VALUE retVal;

    retVal = IRV_TO_PROCESS;
    if (IsHotKeyDigit(sym, state)) {
        if ( input->iCodeInputCount!= 4 ) {
            input->strCodeInput[input->iCodeInputCount++]=sym;
            input->strCodeInput[input->iCodeInputCount]='\0';
            if ( input->iCodeInputCount==4 ) {
                retVal = IRV_TO_PROCESS;
            }
            else
                retVal = IRV_DISPLAY_CANDWORDS;
        }
    }
    else if (IsHotKey(sym, state, FCITX_BACKSPACE)) {
        if (!input->iCodeInputCount)
            return IRV_DONOT_PROCESS_CLEAN;
        input->iCodeInputCount--;
        input->strCodeInput[input->iCodeInputCount] = '\0';

        if (!input->iCodeInputCount)
            retVal = IRV_CLEAN;
        else {
            retVal = IRV_DISPLAY_CANDWORDS;
        }
    }
    else if (IsHotKey(sym, state, FCITX_SPACE)) {
        if (!input->iCodeInputCount)
            return IRV_TO_PROCESS;
        if (input->iCodeInputCount!=3)
            return IRV_DO_NOTHING;

        retVal= CandidateWordChooseByIndex(input->candList, 0);
    }
    else
        return IRV_TO_PROCESS;


    return retVal;
}

INPUT_RETURN_VALUE QWGetCandWord (void *arg, CandidateWord* candWord)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = &qwstate->owner->input;

    strcpy(GetOutputString(input),
           candWord->strWord);
    return IRV_COMMIT_STRING;
}

INPUT_RETURN_VALUE QWGetCandWords (void *arg)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = &qwstate->owner->input;
    int             iQu, iWei;
    int             i;

    CandidateWordSetPageSize(input->candList, 10);
    CandidateWordSetChoose(input->candList, DIGIT_STR_CHOOSE);
    if ( input->iCodeInputCount == 3 )
    {
        iQu = (input->strCodeInput[0] - '0') * 10 + input->strCodeInput[1] - '0';
        iWei = (input->strCodeInput[2]-'0') * 10;

        for (i = 0; i < 10; i++) {
            CandidateWord candWord;
            candWord.callback = QWGetCandWord;
            candWord.owner = qwstate;
            candWord.priv = NULL;
            candWord.strExtra = NULL;
            candWord.strWord = strdup ( GetQuWei (qwstate, iQu, iWei + i + 1) );
            CandidateWordAppend(input->candList, &candWord);
        }
    }
    input->iCursorPos = input->iCodeInputCount;
    SetMessageCount(input->msgPreedit, 0);
    AddMessageAtLast(input->msgPreedit, MSG_INPUT, "%s", input->strCodeInput);

    return IRV_DISPLAY_CANDWORDS;
}

char           *GetQuWei (FcitxQWState* qwstate, int iQu, int iWei)
{
    char *inbuf;
    char *outbuf;

    size_t insize = 2, avail = UTF8_MAX_LENGTH + 1;

    iconv_t convGBK = iconv_open("utf-8", "gb18030");

    if (iQu >= 95) {      /* Process extend Qu 95 and 96 */
        qwstate->strQWHZ[0] = iQu - 95 + 0xA8;
        qwstate->strQWHZ[1] = iWei + 0x40;

        /* skip 0xa87f and 0xa97f */
        if ((unsigned char) qwstate->strQWHZ[1] >= 0x7f)
            qwstate->strQWHZ[1]++;
    }
    else {
        qwstate->strQWHZ[0] = iQu + 0xa0;
        qwstate->strQWHZ[1] = iWei + 0xa0;
    }

    qwstate->strQWHZ[2] = '\0';

    inbuf = qwstate->strQWHZ;

    outbuf = qwstate->strQWHZUTF8;

    iconv(convGBK, &inbuf, &insize, &outbuf, &avail);

    iconv_close(convGBK);

    return qwstate->strQWHZUTF8;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
