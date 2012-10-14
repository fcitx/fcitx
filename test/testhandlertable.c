#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "fcitx-utils/handler-table.h"

#define KEY1 "key1"
#define KEY2 "key-different-length2"
#define KEY_NOT_EXIST "key-not-exist3"

char *strs[] = {
    "jj0", "kjk1", "asdf2", "jjjjj3", "kdjkfa4", "asdf5",
    "asdkf6", "akskfd7", "aksdlfa8", "jaksd9", "jjjjjjakd10", "asdflasd11"
};

int order[] = {11, 10, 9, 8, 7, 6, 0, 1, 2, 3, 4, 5};

int free_count = 0;

typedef struct {
    int id;
    char *str;
} HandlerObj;

static void
test_free_handler_obj(void *obj)
{
    HandlerObj *handler = obj;
    free_count++;
    free(handler->str);
}

int main()
{
    FcitxHandlerTable *table = fcitx_handler_table_new(sizeof(HandlerObj),
                                                       test_free_handler_obj);
    HandlerObj obj;
    int i;
    for (i = 0;i < 6;i++) {
        obj.id = i;
        obj.str = strdup(strs[i]);
        fcitx_handler_table_append_strkey(table, KEY1, &obj);
    }
    for (;i < 12;i++) {
        obj.id = i;
        obj.str = strdup(strs[i]);
        fcitx_handler_table_prepend_strkey(table, KEY1, &obj);
    }
    for (i = 0;i < 6;i++) {
        obj.id = i;
        obj.str = strdup(strs[i]);
        fcitx_handler_table_prepend_strkey(table, KEY2, &obj);
    }
    for (;i < 12;i++) {
        obj.id = i;
        obj.str = strdup(strs[i]);
        fcitx_handler_table_append_strkey(table, KEY2, &obj);
    }

    HandlerObj *p;
    assert(free_count == 0);
    for (p = fcitx_handler_table_first_strkey(table, KEY1), i = 0;
         p;
         p = fcitx_handler_table_next(table, p), i++) {
        assert(i < 12);
        assert(p->id == order[i]);
        assert(strcmp(p->str, strs[p->id]) == 0);
    }
    assert(i == 12);
    fcitx_handler_table_remove_key_strkey(table, KEY1);
    assert(free_count == 12);
    assert(fcitx_handler_table_first_strkey(table, KEY1) == NULL);

    unsigned int id;
    unsigned int next_id;
    id = fcitx_handler_table_last_id_strkey(table, KEY2);
    i = 0;
    for (;(p = fcitx_handler_table_get_by_id(table, id));id = next_id, i++) {
        next_id = fcitx_handler_table_prev_id(table, p);
        assert(i < 12);
        assert(p->id == order[i]);
        assert(strcmp(p->str, strs[p->id]) == 0);
        fcitx_handler_table_remove_by_id(table, id);
        assert(free_count == i + 13);
    }
    assert(i == 12);
    assert(free_count == 24);
    assert(fcitx_handler_table_first_id_strkey(table, KEY2) ==
           FCITX_OBJECT_POOL_INVALID_ID);
    assert(fcitx_handler_table_last_id_strkey(table, KEY2) ==
           FCITX_OBJECT_POOL_INVALID_ID);
    assert(fcitx_handler_table_first_strkey(table, KEY_NOT_EXIST) == NULL);
    assert(fcitx_handler_table_last_strkey(table, KEY_NOT_EXIST) == NULL);
    fcitx_handler_table_remove_by_id(table, FCITX_OBJECT_POOL_INVALID_ID);
    fcitx_handler_table_remove_key_strkey(table, KEY_NOT_EXIST);

    fcitx_handler_table_free(table);
    return 0;
}
