
#ifndef TABLEPINYINWRAPPER_H
#define TABLEPINYINWRAPPER_H

#include "fcitx/addon.h"
#include "fcitx-config/hotkey.h"
#include "fcitx/ime.h"

void Table_LoadPYBaseDict(FcitxAddon* pyaddon);
void Table_PYGetPYByHZ(FcitxAddon* pyaddon, char *a, char* b);
void Table_PYGetCandWord(FcitxAddon* pyaddon, int a);
void Table_DoPYInput(FcitxAddon* pyaddon, FcitxKeySym sym, unsigned int state);
void Table_PYGetCandWords(FcitxAddon* pyaddon, SEARCH_MODE mode);
void Table_PYGetCandText(FcitxAddon* pyaddon, int iIndex, char *strText);
void Table_ResetPY(FcitxAddon* pyaddon);
char* Table_PYGetFindString(FcitxAddon* pyaddon);

#endif