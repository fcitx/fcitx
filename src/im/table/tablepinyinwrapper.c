
#include "fcitx/module.h"
#include "fcitx/addon.h"
#include "im/pinyin/pydef.h"
#include "tablepinyinwrapper.h"
#include "table.h"
#include "fcitx/instance.h"

void Table_LoadPYBaseDict(FcitxAddon* pyaddon)
{
    FcitxModuleFunctionArg args;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_LOADBASEDICT, args);
}

void Table_PYGetPYByHZ(FcitxAddon* pyaddon, char *a, char* b)
{
    FcitxModuleFunctionArg args;
    args.args[0] = a;
    args.args[1] = b;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETPYBYHZ, args);
}

void Table_DoPYInput(FcitxAddon* pyaddon, FcitxKeySym sym, unsigned int state)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &sym;
    args.args[1] = &state;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_DOPYINPUT, args);
}

void Table_PYGetCandWords(FcitxAddon* pyaddon)
{
    FcitxModuleFunctionArg args;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETCANDWORDS, args);
}

void Table_ResetPY(FcitxAddon* pyaddon)
{
    FcitxModuleFunctionArg args;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYRESET, args);
}

char* Table_PYGetFindString(FcitxAddon* pyaddon)
{
    FcitxModuleFunctionArg args;
    char * str = InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETFINDSTRING, args);
    return str;
}

INPUT_RETURN_VALUE Table_PYGetCandWord(void* arg, CandidateWord* candidateWord)
{
    FcitxTableState* tbl = arg;
    INPUT_RETURN_VALUE retVal = tbl->pygetcandword(tbl->pyaddon->addonInstance, candidateWord);
    Table_ResetPY(tbl->pyaddon);
    if (!(retVal & IRV_FLAG_PENDING_COMMIT_STRING))
    {
        strcpy(GetOutputString(&tbl->owner->input), candidateWord->strWord);
    }

    return IRV_COMMIT_STRING | IRV_FLAG_RESET_INPUT;
}
// kate: indent-mode cstyle; space-indent on; indent-width 0; 
