#include "fcitx-config/fcitx-config.h"


int main(int argc, char* argv[])
{
    if (argc <= 3)
        return 1;
    
    
    ConfigFileDesc* configDesc = ParseConfigFileDesc(argv[1]);
    GenericConfig gc;
    gc.configFile = ParseConfigFile(argv[2], configDesc);
    SaveConfigFile(argv[3], &gc, configDesc);
    return 0;
}
