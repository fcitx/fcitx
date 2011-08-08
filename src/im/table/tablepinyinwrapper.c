
#include "fcitx/module.h"
#include "fcitx/addon.h"
#include "im/pinyin/pydef.h"
#include "tablepinyinwrapper.h"

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

void Table_PYGetCandWord(FcitxAddon* pyaddon, int a)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &a;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETCANDWORD, args);
}

void Table_DoPYInput(FcitxAddon* pyaddon, FcitxKeySym sym, unsigned int state)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &sym;
    args.args[1] = &state;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_DOPYINPUT, args);
}

void Table_PYGetCandWords(FcitxAddon* pyaddon, SEARCH_MODE mode)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &mode;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETCANDWORDS, args);
}

void Table_PYGetCandText(FcitxAddon* pyaddon, int iIndex, char *strText)
{
    FcitxModuleFunctionArg args;
    args.args[0] = &iIndex;
    args.args[1] = strText;
    InvokeModuleFunction(pyaddon, FCITX_PINYIN_PYGETCANDTEXT, args);
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
