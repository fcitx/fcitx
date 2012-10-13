#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "fcitx-utils/objpool.h"

int main()
{
    FcitxObjPool* pool = fcitx_obj_pool_new(4);
    int id[32];

    int i;
    for (i = 0; i < 32; i++) {
        id[i] = fcitx_obj_pool_alloc_id(pool);
        assert(id[i] >= 0);
        int32_t* data = (int32_t*) fcitx_obj_pool_get(pool, id[i]);
        *data = i;
    }

    for (i = 0; i < 32; i+=2) {
        assert(fcitx_obj_pool_free_id(pool, id[i]));
    }

    for (i = 0; i < 32; i+=2) {
        assert(!fcitx_obj_pool_free_id(pool, id[i]));
    }

    for (i = 0; i < 32; i+=2) {
        id[i] = fcitx_obj_pool_alloc_id(pool);
        assert(id[i] >= 0);
        int32_t* data = (int32_t*) fcitx_obj_pool_get(pool, id[i]);
        *data = i;
    }

    for (i = 1; i < 32; i+=2) {
        int32_t* data = (int32_t*) fcitx_obj_pool_get(pool, id[i]);
        assert(*data == i);
        assert(fcitx_obj_pool_free_id(pool, id[i]));
    }

    fcitx_obj_pool_free(pool);

    return 0;
}