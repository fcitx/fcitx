#include "fcitx-qt/fcitxqtkeysequencewidget.h"
#include <libintl.h>
#include <locale.h>
#include <fcitx-utils/utils.h>
#include <QApplication>

int main(int argc, char* argv[])
{
    char* localedir = fcitx_utils_get_fcitx_path("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir);
    free(localedir);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");

    QApplication app(argc, argv);

    FcitxQtKeySequenceWidget w;
    w.setMultiKeyShortcutsAllowed(false);
    w.setModifierOnlyAllowed(true);
    w.show();

    app.exec();
}
