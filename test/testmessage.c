#include <assert.h>
#include "fcitx/instance.h"
#include "fcitx/candidate.h"
#include "fcitx/instance-internal.h"
#include "fcitx/ime-internal.h"

int main()
{
    char* words[] = { "a", "b", "c" , "d", "e" };
    char* extras[] = { "A", "B", "C" , "D", "E" };
    FcitxCandidateWordList* candList = FcitxCandidateWordNewList();
    FcitxInstance* instance = fcitx_utils_malloc0(sizeof(FcitxInstance));
    instance->input = fcitx_utils_malloc0(sizeof(FcitxInputState));
    instance->input->candList = FcitxCandidateWordNewList();
    instance->config = fcitx_utils_malloc0(sizeof(FcitxGlobalConfig));
    instance->config->bPointAfterNumber = true;
    FcitxCandidateWord word;
    word.callback = NULL;
    word.owner = NULL;
    word.priv = NULL;
    int i = 0;
    for (i = 0; i < 5; i ++) {
        word.strWord = strdup(words[i]);
        word.strExtra = strdup(extras[i]);
        FcitxCandidateWordAppend(instance->input->candList, &word);
    }
    for (i = 0; i < 4; i ++) {
        word.strWord = strdup(words[i]);
        word.strExtra = strdup(extras[i]);
        FcitxCandidateWordAppend(candList, &word);
    }

    char* result = FcitxUICandidateWordToCString(instance);
    assert (strcmp(result, "1.aA 2.bB 3.cC 4.dD 5.eE ") == 0);
    free(result);

    FcitxCandidateWordSetPageSize(instance->input->candList, 10);
    FcitxCandidateWordMerge(instance->input->candList, candList, 2);
    result = FcitxUICandidateWordToCString(instance);
    assert (strcmp(result, "1.aA 2.bB 3.aA 4.bB 5.cC 6.dD "
                   "7.cC 8.dD 9.eE ") == 0);
    free(result);

    FcitxCandidateWordFreeList(instance->input->candList);
    FcitxCandidateWordFreeList(candList);
    free(instance->config);
    free(instance->input);
    free(instance);


    return 0;
}
