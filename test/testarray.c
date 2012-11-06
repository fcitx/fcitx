#include <assert.h>
#include "fcitx-utils/utils.h"
#include "fcitx/fcitx.h"

int main()
{
    UT_array array;
    utarray_init(&array, &ut_int_icd);
    int test[] = {3, 1, 2};
    int test2[] = {1, 2, 3};
    FCITX_UNUSED(test);
    FCITX_UNUSED(test2);
    int i;
    i = 1;
    utarray_push_back(&array, &i);
    i = 2;
    utarray_push_back(&array, &i);
    i = 3;
    utarray_push_back(&array, &i);
    utarray_move(&array, 2, 0);
    {
        utarray_foreach(p, &array, int) {
            assert(*p == test[utarray_eltidx(&array, p)]);
        }
    }
    utarray_move(&array, 0, 2);

    {
        utarray_foreach(p, &array, int) {
            assert(*p == test2[utarray_eltidx(&array, p)]);
        }
    }
    utarray_done(&array);

    return 0;
}
