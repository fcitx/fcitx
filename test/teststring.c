#include <assert.h>
#include "fcitx-utils/utils.h"

int main()
{
    const char *test = "a,b,c,d";

    UT_array* list = fcitx_utils_split_string(test, ',');
    assert(utarray_len(list) == 4);
    char* join = fcitx_utils_join_string_list(list, ',');
    assert(strcmp(join, test) == 0);

    char* cat = NULL;
    fcitx_utils_alloc_cat_str(cat, join, ",e");
    assert(strcmp(cat, "a,b,c,d,e") == 0);

    free(cat);

    free(join);
    fcitx_utils_free_string_list(list);

    char localcat[20];
    const char *array[] = {"a", ",b", ",c", ",d"};
    fcitx_utils_cat_str_simple(localcat, 4, array);
    assert(strcmp(localcat, test) == 0);

    char localcat2[6];
    fcitx_utils_cat_str_simple_with_len(localcat2, 4, 4, array);
    assert(strcmp(localcat2, "a,b") == 0);

    fcitx_utils_cat_str_simple_with_len(localcat2, 5, 4, array);
    assert(strcmp(localcat2, "a,b,") == 0);

    fcitx_utils_cat_str_simple_with_len(localcat2, 6, 4, array);
    assert(strcmp(localcat2, "a,b,c") == 0);


    return 0;
}