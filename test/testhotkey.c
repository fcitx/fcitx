#include <assert.h>
#include "fcitx-config/hotkey.h"

#define TEST_HOTKEY_UNIFICATION(ORGSYM, ORGSTATE, OBJSYM, OBJSTATE) \
    do { \
        FcitxKeySym sym = ORGSYM; \
        unsigned int state = ORGSTATE; \
        FcitxHotkeyGetKey(sym, state, &sym, &state); \
        assert(OBJSYM == sym); \
        assert(OBJSTATE == state); \
    } while(0)

int main()
{
    TEST_HOTKEY_UNIFICATION(FcitxKey_a, 0, FcitxKey_a, 0);
    TEST_HOTKEY_UNIFICATION(FcitxKey_A, FcitxKeyState_Shift, FcitxKey_A, 0);
    TEST_HOTKEY_UNIFICATION(FcitxKey_F, FcitxKeyState_Ctrl_Shift, FcitxKey_F, FcitxKeyState_Ctrl_Shift);
    TEST_HOTKEY_UNIFICATION(FcitxKey_F, FcitxKeyState_Ctrl, FcitxKey_F, FcitxKeyState_Ctrl);
    TEST_HOTKEY_UNIFICATION(FcitxKey_f, FcitxKeyState_Ctrl_Shift, FcitxKey_F, FcitxKeyState_Ctrl_Shift);
    TEST_HOTKEY_UNIFICATION(FcitxKey_f, FcitxKeyState_Ctrl_Alt, FcitxKey_F, FcitxKeyState_Ctrl_Alt);

    return 0;
}
