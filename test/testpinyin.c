#include <assert.h>
#include <fcitx/fcitx.h>

#include "pyParser.h"
#include "pyconfig.h"
#include "PYFA.h"


void PrintParsedPY(ParsePYStruct *parse, const char *expect)
{
    FCITX_UNUSED(expect);
    /* only test, so don't care too much about size */
    int i;
    char* buf = fcitx_utils_malloc0(1024);
    for (i = 0; i < parse->iHZCount; i ++) {
        if (i != 0)
            strcat(buf, " ");
        strcat(buf, parse->strPYParsed[i]);
    }
    fprintf(stderr, "%s\n", buf);
    assert(strcmp(expect, buf) == 0);
    free(buf);
}

int main()
{
    FcitxPinyinConfig pyconfig;
    ParsePYStruct parse;
    memset(&pyconfig, 0, sizeof(pyconfig));
    memset(&parse, 0, sizeof(parse));
    InitPYTable(&pyconfig);
    InitPYSplitData(&pyconfig);
    ParsePY(&pyconfig, "wanan", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "wan an");
    ParsePY(&pyconfig, "dier", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "di er");
    ParsePY(&pyconfig, "dierge", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "di er ge");
    ParsePY(&pyconfig, "dieru", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "die ru");
    ParsePY(&pyconfig, "diepian", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "die pian");
    ParsePY(&pyconfig, "bier", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "bi er");
    ParsePY(&pyconfig, "bingan", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "bing an");
    ParsePY(&pyconfig, "xiai", &parse, PY_PARSE_INPUT_USER, false);
    PrintParsedPY(&parse, "xi ai");
    return 0;
}
