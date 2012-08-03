#include <assert.h>
#include "fcitx-config/fcitx-config.h"
#include "fcitx-config/hotkey.h"

typedef struct _TestConfig
{
    FcitxGenericConfig gc;
    char* name;
    char* str;
    FcitxHotkey hk[2];
    char c;
    int i;
    boolean b;
} TestConfig;

CONFIG_BINDING_BEGIN(TestConfig)
CONFIG_BINDING_REGISTER("Test", "Name", name)
CONFIG_BINDING_REGISTER("Test", "String", str)
CONFIG_BINDING_REGISTER("Test", "Hotkey", hk)
CONFIG_BINDING_REGISTER("Test", "Char", c)
CONFIG_BINDING_REGISTER("Test", "Integer", i)
CONFIG_BINDING_REGISTER("Test", "Boolean", b)
CONFIG_BINDING_END()


int main(int argc, char* argv[])
{
    if (argc <= 3)
        return 1;

    FcitxConfigFileDesc* configDesc = FcitxConfigParseConfigFileDesc(argv[1]);
    assert(configDesc);
    TestConfig tc;
    memset(&tc, 0, sizeof(tc));
    FcitxConfigFile* configFile = FcitxConfigParseConfigFile(argv[2], configDesc);
    assert(configFile);
    TestConfigConfigBind(&tc, configFile, configDesc);
    FcitxConfigBindSync((FcitxGenericConfig*) &tc);

    assert(tc.i == 1);
    assert(strcmp(tc.str, "CTRL_A") == 0);
    assert(tc.hk[0].sym == FcitxKey_A);
    assert(tc.hk[0].state == FcitxKeyState_Ctrl);
    assert(tc.hk[1].sym == FcitxKey_None);
    assert(tc.hk[1].state == FcitxKeyState_None);
    assert(tc.c == 'a');
    assert(tc.b == true);


    FcitxConfigSaveConfigFile(argv[3], (FcitxGenericConfig*) &tc, configDesc);

    FcitxConfigFree((FcitxGenericConfig*) &tc);

    FcitxConfigFreeConfigFileDesc(configDesc);
    return 0;
}
