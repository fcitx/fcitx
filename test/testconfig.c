#include "fcitx-config/fcitx-config.h"


int main(int argc, char* argv[])
{
    if (argc <= 3)
        return 1;


    FcitxConfigFileDesc* configDesc = FcitxConfigParseConfigFileDesc(argv[1]);
    FcitxGenericConfig gc;
    gc.configFile = FcitxConfigParseConfigFile(argv[2], configDesc);
    FcitxConfigSaveConfigFile(argv[3], &gc, configDesc);
    return 0;
}
