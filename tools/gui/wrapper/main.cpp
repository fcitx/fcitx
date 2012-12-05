#include <fcitx-utils/utils.h>
#include <libintl.h>
#include "wrapperapp.h"

int main(int argc, char* argv[])
{
    char* localedir = fcitx_utils_get_fcitx_path("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir);
    free(localedir);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");
    WrapperApp app(argc, argv);
    return app.exec();
}
