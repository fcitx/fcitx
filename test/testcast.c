#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "fcitx-utils/utils.h"
#include "fcitx/fcitx.h"

typedef struct {
    char str[3];
} TestType;

int
main()
{
    void *p;
    void *p1;

    /**
     * test if the type casting function works for float,
     * which is what these macros / functions is designed for.
     **/
    float f0 = 1.123;
    p = fcitx_utils_float_to_ptr(f0);
    float f1 = fcitx_utils_ptr_to_float(p);
    p1 = fcitx_utils_float_to_ptr(f1);
    f1 = fcitx_utils_ptr_to_float(p1);
    assert(p1 == p);
    assert(f0 == f1);

    TestType t0 = {
        .str = "\0as"
    };
    _FCITX_CAST_TO_PTR(p, TestType, t0);
    TestType t1;
    _FCITX_CAST_FROM_PTR(TestType, t1, p);
    _FCITX_CAST_TO_PTR(p1, TestType, t1);
    _FCITX_CAST_FROM_PTR(TestType, t1, p1);
    assert(p1 == p);
    assert(memcmp(&t1, &t0, sizeof(t1)) == 0);

    /**
     * test if the type casting function for int is compatible with
     * the simple type casting. The pointer from the two methods may be
     * different, but what we care about is whether casting it back can
     * always reproduce the same result.
     **/
#define def_test_type(name, type, val) do {                             \
        type test_##name##_var1 = (type)(val);                          \
        void *test_##name##_ptr1 = (void*)(intptr_t)test_##name##_var1; \
        void *test_##name##_ptr2 = fcitx_utils_##name##_to_ptr(         \
            test_##name##_var1);                                        \
        type test_##name##_var2 = fcitx_utils_ptr_to_##name(            \
            test_##name##_ptr1);                                        \
        type test_##name##_var3 = fcitx_utils_ptr_to_##name(            \
            test_##name##_ptr2);                                        \
        type test_##name##_var4 = (type)(intptr_t)test_##name##_ptr1;   \
        type test_##name##_var5 = (type)(intptr_t)test_##name##_ptr2;   \
        assert(test_##name##_var2 == test_##name##_var1);               \
        assert(test_##name##_var3 == test_##name##_var1);               \
        assert(test_##name##_var4 == test_##name##_var1);               \
        assert(test_##name##_var5 == test_##name##_var1);               \
        FCITX_UNUSED(test_##name##_var2);                               \
        FCITX_UNUSED(test_##name##_var3);                               \
        FCITX_UNUSED(test_##name##_var4);                               \
        FCITX_UNUSED(test_##name##_var5);                               \
    } while (0)

    def_test_type(int, int, -3);
    def_test_type(intptr, intptr_t, -3);
    def_test_type(int8, int8_t, -3);
    def_test_type(int16, int16_t, -3);
    def_test_type(int32, int32_t, -3);
    def_test_type(uint, unsigned int, -3);
    def_test_type(uintptr, uintptr_t, -3);
    def_test_type(uint8, uint8_t, -3);
    def_test_type(uint16, uint16_t, -3);
    def_test_type(uint32, uint32_t, -3);

    def_test_type(int, int, 71);
    def_test_type(intptr, intptr_t, 71);
    def_test_type(int8, int8_t, 71);
    def_test_type(int16, int16_t, 71);
    def_test_type(int32, int32_t, 71);
    def_test_type(uint, unsigned int, 71);
    def_test_type(uintptr, uintptr_t, 71);
    def_test_type(uint8, uint8_t, 71);
    def_test_type(uint16, uint16_t, 71);
    def_test_type(uint32, uint32_t, 71);

    def_test_type(int, int, 71893);
    def_test_type(intptr, intptr_t, 71893);
    def_test_type(int8, int8_t, 71893);
    def_test_type(int16, int16_t, 71893);
    def_test_type(int32, int32_t, 71893);
    def_test_type(uint, unsigned int, 71893);
    def_test_type(uintptr, uintptr_t, 71893);
    def_test_type(uint8, uint8_t, 71893);
    def_test_type(uint16, uint16_t, 71893);
    def_test_type(uint32, uint32_t, 71893);

    def_test_type(int, int, -1732493);
    def_test_type(intptr, intptr_t, -1732493);
    def_test_type(int8, int8_t, -1732493);
    def_test_type(int16, int16_t, -1732493);
    def_test_type(int32, int32_t, -1732493);
    def_test_type(uint, unsigned int, -1732493);
    def_test_type(uintptr, uintptr_t, -1732493);
    def_test_type(uint8, uint8_t, -1732493);
    def_test_type(uint16, uint16_t, -1732493);
    def_test_type(uint32, uint32_t, -1732493);

    return 0;
}
