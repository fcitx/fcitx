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
#include "fcitx/context.h"

typedef struct _FcitxQWState {
    char     strQWHZ[3];
    char     strQWHZUTF8[UTF8_MAX_LENGTH + 1];
    FcitxInstance *owner;
} FcitxQWState;

static void* QWCreate(struct _FcitxInstance* instance);
INPUT_RETURN_VALUE DoQWInput(void* arg, FcitxKeySym sym, unsigned int state);
INPUT_RETURN_VALUE QWGetCandWords(void *arg);
INPUT_RETURN_VALUE QWGetCandWord(void *arg, FcitxCandidateWord* candWord);
char           *GetQuWei(FcitxQWState* qwstate, int iQu, int iWei);
boolean QWInit(void *arg);

FCITX_EXPORT_API
FcitxIMClass ime = {
    QWCreate,
    NULL
};

FCITX_EXPORT_API
int ABI_VERSION = FCITX_ABI_VERSION;

void* QWCreate(struct _FcitxInstance* instance)
{
    FcitxQWState* qwstate = fcitx_utils_malloc0(sizeof(FcitxQWState));
    FcitxInstanceRegisterIM(
        instance,
        qwstate,
        "quwei",
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
        100 /* make quwei place at last */,
        "zh_CN"
    );
    qwstate->owner = instance;
    return qwstate;
}

boolean QWInit(void *arg)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInstanceSetContext(qwstate->owner, CONTEXT_IM_KEYBOARD_LAYOUT, "us");
    return true;
}

INPUT_RETURN_VALUE DoQWInput(void* arg, FcitxKeySym sym, unsigned int state)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(qwstate->owner);
    char* strCodeInput = FcitxInputStateGetRawInputBuffer(input);
    INPUT_RETURN_VALUE retVal;

    retVal = IRV_TO_PROCESS;
    if (FcitxHotkeyIsHotKeyDigit(sym, state)) {
        if (FcitxInputStateGetRawInputBufferSize(input) != 4) {
            strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = sym;
            strCodeInput[FcitxInputStateGetRawInputBufferSize(input) + 1] = '\0';
            FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) + 1);
            if (FcitxInputStateGetRawInputBufferSize(input) == 4) {
                retVal = IRV_TO_PROCESS;
            } else
                retVal = IRV_DISPLAY_CANDWORDS;
        }
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_BACKSPACE)) {
        if (!FcitxInputStateGetRawInputBufferSize(input))
            return IRV_DONOT_PROCESS_CLEAN;
        FcitxInputStateSetRawInputBufferSize(input, FcitxInputStateGetRawInputBufferSize(input) - 1);
        strCodeInput[FcitxInputStateGetRawInputBufferSize(input)] = '\0';

        if (!FcitxInputStateGetRawInputBufferSize(input))
            retVal = IRV_CLEAN;
        else {
            retVal = IRV_DISPLAY_CANDWORDS;
        }
    } else if (FcitxHotkeyIsHotKey(sym, state, FCITX_SPACE)) {
        if (!FcitxInputStateGetRawInputBufferSize(input))
            return IRV_TO_PROCESS;
        if (FcitxInputStateGetRawInputBufferSize(input) != 3)
            return IRV_DO_NOTHING;

        retVal = FcitxCandidateWordChooseByIndex(FcitxInputStateGetCandidateList(input), 0);
    } else
        return IRV_TO_PROCESS;


    return retVal;
}

INPUT_RETURN_VALUE QWGetCandWord(void *arg, FcitxCandidateWord* candWord)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(qwstate->owner);

    strcpy(FcitxInputStateGetOutputString(input),
           candWord->strWord);
    return IRV_COMMIT_STRING;
}

INPUT_RETURN_VALUE QWGetCandWords(void *arg)
{
    FcitxQWState* qwstate = (FcitxQWState*) arg;
    FcitxInputState* input = FcitxInstanceGetInputState(qwstate->owner);
    int             iQu, iWei;
    int             i;

    FcitxCandidateWordSetPageSize(FcitxInputStateGetCandidateList(input), 10);
    FcitxCandidateWordSetChoose(FcitxInputStateGetCandidateList(input), DIGIT_STR_CHOOSE);
    if (FcitxInputStateGetRawInputBufferSize(input) == 3) {
        iQu = (FcitxInputStateGetRawInputBuffer(input)[0] - '0') * 10 + FcitxInputStateGetRawInputBuffer(input)[1] - '0';
        iWei = (FcitxInputStateGetRawInputBuffer(input)[2] - '0') * 10;

        for (i = 0; i < 10; i++) {
            FcitxCandidateWord candWord;
            candWord.callback = QWGetCandWord;
            candWord.owner = qwstate;
            candWord.priv = NULL;
            candWord.strExtra = NULL;
            candWord.strWord = strdup(GetQuWei(qwstate, iQu, iWei + i + 1));
            candWord.wordType = MSG_OTHER;
            FcitxCandidateWordAppend(FcitxInputStateGetCandidateList(input), &candWord);
        }
    }
    FcitxInputStateSetCursorPos(input, FcitxInputStateGetRawInputBufferSize(input));
    FcitxMessagesSetMessageCount(FcitxInputStateGetPreedit(input), 0);
    FcitxMessagesAddMessageAtLast(FcitxInputStateGetPreedit(input), MSG_INPUT, "%s", FcitxInputStateGetRawInputBuffer(input));

    return IRV_DISPLAY_CANDWORDS;
}

char           *GetQuWei(FcitxQWState* qwstate, int iQu, int iWei)
{
    IconvStr inbuf;
    char *outbuf;

    size_t insize = 2, avail = UTF8_MAX_LENGTH + 1;

    iconv_t convGBK = iconv_open("utf-8", "gb18030");

    if (iQu >= 95) {      /* Process extend Qu 95 and 96 */
        qwstate->strQWHZ[0] = iQu - 95 + 0xA8;
        qwstate->strQWHZ[1] = iWei + 0x40;

        /* skip 0xa87f and 0xa97f */
        if ((unsigned char) qwstate->strQWHZ[1] >= 0x7f)
            qwstate->strQWHZ[1]++;
    } else {
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
