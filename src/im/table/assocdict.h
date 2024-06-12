#ifndef ASSOCDICT_H
#define ASSOCDICT_H

#include "fcitx/candidate.h"
#include "fcitx/instance.h"
#include "fcitx-utils/memory.h"
#include "fcitx-utils/utarray.h"
#include "fcitx-utils/uthash.h"

struct AssocDict;
struct AssocDictList;

const struct AssocDictList* TableCreateAssocDictList();
const struct AssocDict* TableFindAssocDictFromLangCode(
        const struct AssocDictList* list, const char* langCode);
void TableGetAssocPhrases(FcitxInstance* instance,
        FcitxCandidateWordList* cand_list, int count,
        const struct AssocDict* dict, const char* str);

#endif
// kate: indent-mode cstyle; space-indent on; indent-width 0;
