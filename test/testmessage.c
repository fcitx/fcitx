#include "fcitx/instance.h"
#include "fcitx/candidate.h"
#include "fcitx/instance-internal.h"
#include "fcitx/ime-internal.h"

int main()
{
    char* words[] = { "a", "b", "c" , "d", "e" };
    char* extras[] = { "A", "B", "C" , "D", "E" };
    FcitxInstance* instance = fcitx_malloc0(sizeof(FcitxInstance));
    instance->input->candList = CandidateWordInit();
    instance->config = fcitx_malloc0(sizeof(FcitxConfig));
    instance->config->bPointAfterNumber = true;
    CandidateWord word;
    word.callback = NULL;
    word.owner = NULL;
    word.priv = NULL;
    int i = 0;
    for (i = 0; i < 5; i ++) {
        word.strWord = strdup(words[i]);
        word.strExtra = strdup(extras[i]);
        CandidateWordAppend(instance->input->candList, &word);
    }

    char* result = CandidateWordToCString(instance);
    if (strcmp(result, "1.aA 2.bB 3.cC 4.dD 5.eE ") == 0) {
        return 0;
    }

    return 1;
}