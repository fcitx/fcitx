#include <assert.h>
#include <string.h>

#include <fcitx-utils/utils.h>
#include "charselectdata.h"


CharSelectData* CharSelectDataCreateFromFile(const char* f)
{
    CharSelectData* charselect = fcitx_utils_new(CharSelectData);

    do {

        FILE* fp = fopen(f, "r");
        if (!fp)
            break;

        fseek(fp, 0, SEEK_END);
        long int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        charselect->size = size;
        charselect->dataFile = fcitx_utils_malloc0(size);
        fread(charselect->dataFile, 1, size, fp);

        fclose(fp);

        CharSelectDataCreateIndex(charselect);

        return charselect;
    } while(0);

    free(charselect);
    return NULL;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;
    CharSelectData* charselect = CharSelectDataCreateFromFile(argv[1]);
    unsigned int i;

    CharSelectDataDump(charselect);

    UT_array* aliases = CharSelectDataAliases(charselect, 0x0021);
    fprintf(stderr, "Aliases:\n");
    for(i = 0; i < utarray_len(aliases); i ++) {
        fprintf(stderr, "  %s\n", *(char**) utarray_eltptr(aliases, i));
    }
    fcitx_utils_free_string_list(aliases);

    UT_array* notes = CharSelectDataNotes(charselect, 0x018D);
    fprintf(stderr, "Notes:\n");
    for(i = 0; i < utarray_len(notes); i ++) {
        fprintf(stderr, "  %s\n", *(char**) utarray_eltptr(notes, i));
    }
    fcitx_utils_free_string_list(notes);

    UT_array* equivalents = CharSelectDataEquivalents(charselect, 0x2000);
    fprintf(stderr, "Equivalents:\n");
    for(i = 0; i < utarray_len(equivalents); i ++) {
        fprintf(stderr, "  %s\n", *(char**) utarray_eltptr(equivalents, i));
    }
    fcitx_utils_free_string_list(equivalents);

    UT_array* approximateEquivalents = CharSelectDataApproximateEquivalents(charselect, 0x2003);
    fprintf(stderr, "Approximate Equivalents:\n");
    for(i = 0; i < utarray_len(approximateEquivalents); i ++) {
        fprintf(stderr, "  %s\n", *(char**) utarray_eltptr(approximateEquivalents, i));
    }
    fcitx_utils_free_string_list(approximateEquivalents);

    UT_array* unihan = CharSelectDataUnihanInfo(charselect, 0x6107);
    char* label[] = {
        "Definition in English: ",
        "Cantonese Pronunciation: ",
        "Mandarin Pronunciation: ",
        "Tang Pronunciation: ",
        "Korean Pronunciation: ",
        "Japanese On Pronunciation: ",
        "Japanese Kun Pronunciation: ",
    };
    for(i = 0; i < utarray_len(unihan); i ++) {
        fprintf(stderr, "%s: %s\n", label[i], *(char**) utarray_eltptr(unihan, i));
    }
    fcitx_utils_free_string_list(unihan);

    UT_array* result = CharSelectDataFind(charselect, "f");
    utarray_foreach(c, result, uint16_t) {
        fprintf(stderr, "%x\n", *c);
    }

    utarray_free(result);

    CharSelectDataFree(charselect);

    return 0;
}
