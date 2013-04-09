#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include "fcitx-utils/utarray.h"

typedef struct {
    int a;
    int b;
} SortItem;

SortItem array[] = {
    {3, 4},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
};

SortItem msort_array[] = {
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {2, 4},
    {2, 3},
    {3, 4},
};


int cmp(const void* a, const void* b, void* thunk)
{
    return ((SortItem*)a)->a - ((SortItem*)b)->a;
}

int intcmp(const void* a, const void* b, void* thunk)
{
    if (thunk) {
        return (*((int*)b)) - (*((int*)a));
    }
    return (*((int*)a)) - (*((int*)b));
}

#define _AS(ARRAY) (sizeof(ARRAY) / sizeof(ARRAY[0]))

int main()
{
    /* stable test */
    fcitx_msort_r(array, _AS(array), sizeof(array[0]), cmp, NULL);

    int i = 0;
    for (i = 0; i < _AS(array); i ++) {
        assert(array[i].a == msort_array[i].a);
        assert(array[i].b == msort_array[i].b);
    }

    srand(time(NULL));

    {
        /* qsort_test */
        int qsort_intarray[1024];
        for (i = 0; i <_AS(qsort_intarray); i ++) {
            qsort_intarray[i] = rand();
        }
        fcitx_qsort_r(qsort_intarray, _AS(qsort_intarray), sizeof(qsort_intarray[0]), intcmp, NULL);

        for (i = 0; i < _AS(qsort_intarray) - 1; i ++) {
            assert(qsort_intarray[i] <= qsort_intarray[i + 1]);
        }

        /* msort_test */
        fcitx_msort_r(qsort_intarray, _AS(qsort_intarray), sizeof(qsort_intarray[0]), intcmp, (void*) 0x1);

        for (i = 0; i < _AS(qsort_intarray) - 1; i ++) {
            assert(qsort_intarray[i] >= qsort_intarray[i + 1]);
        }
    }

    {
        /* qsort_test */
        int qsort_intarray[6];
        for (i = 0; i <_AS(qsort_intarray); i ++) {
            qsort_intarray[i] = rand();
        }
        fcitx_qsort_r(qsort_intarray, _AS(qsort_intarray), sizeof(qsort_intarray[0]), intcmp, (void*) 0x1);

        for (i = 0; i < _AS(qsort_intarray) - 1; i ++) {
            assert(qsort_intarray[i] >= qsort_intarray[i + 1]);
        }

        /* msort_test */
        fcitx_msort_r(qsort_intarray, _AS(qsort_intarray), sizeof(qsort_intarray[0]), intcmp, NULL);

        for (i = 0; i < _AS(qsort_intarray) - 1; i ++) {
            assert(qsort_intarray[i] <= qsort_intarray[i + 1]);
        }
    }

    return 0;
}
