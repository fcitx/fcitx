
#include "core/fcitx.h"
#include "utils/utils.h"
#include "utils/configfile.h"
#include "core/addon.h"
#include "core/ime-internal.h"
#include "core/backend.h"
#include "core/module.h"
#include <locale.h>

FcitxConfig fc;
FcitxState gs;

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(PACKAGE, "UTF-8");
    textdomain(PACKAGE);

	LoadConfig();
	LoadAddonInfo();
    AddonResolveDependency();
    
    FcitxInitThread();
    InitAsDaemon();
    LoadModule();
    LoadAllIM();
    StartBackend();
	return 0;
}