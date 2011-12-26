#include "fcitx-utils/utils.h"
#include "assert.h"

int main()
{
    char *test = "a,b,c,d";
    
    UT_array* list = fcitx_utils_split_string(test, ',');
    assert(utarray_len(list) == 4);
    char* join = fcitx_utils_join_string_list(list, ',');
    assert(strcmp(join, test) == 0);
    free(join);
    fcitx_utils_free_string_list(list);
    
    return 0;
}