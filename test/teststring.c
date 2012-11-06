#include <assert.h>
#include "fcitx-utils/utils.h"

#define TEST_STR "a,b,c,d"

int main()
{
    const char *test = TEST_STR;

    UT_array* list = fcitx_utils_split_string(test, ',');
    assert(utarray_len(list) == 4);
    char* join = fcitx_utils_join_string_list(list, ',');
    assert(strcmp(join, test) == 0);
    fcitx_utils_append_split_string(list, TEST_STR, "\n");
    char *join2 = fcitx_utils_join_string_list(list, ',');
    assert(strcmp(join2, TEST_STR","TEST_STR) == 0);

    char* cat = NULL;
    fcitx_utils_alloc_cat_str(cat, join, ",e");
    assert(strcmp(cat, TEST_STR",e") == 0);
    fcitx_utils_set_cat_str(cat, join, ",e,", join);
    assert(strcmp(cat, TEST_STR",e,"TEST_STR) == 0);
    join = fcitx_utils_set_str(join, join2);
    assert(strcmp(join, join2) == 0);

    free(cat);
    free(join);
    free(join2);
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

    const char *orig = "\r78\"1\n\\\e\\3\f\a\v\'cc\td\b";
    char *escape = fcitx_utils_set_escape_str(NULL, orig);
    assert(strcmp(escape,
                  "\\r78\\\"1\\n\\\\\\e\\\\3\\f\\a\\v\\\'cc\\td\\b") == 0);
    char *back = fcitx_utils_set_unescape_str(NULL, escape);
    assert(strcmp(orig, back) == 0);
    fcitx_utils_unescape_str_inplace(escape);
    assert(strcmp(orig, escape) == 0);
    free(escape);
    free(back);

    return 0;
}
