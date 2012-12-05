#include "fcitx-qt/fcitxqtconfiguiplugin.h"

class QuickPhraseEditorPlugin : public FcitxQtConfigUIPlugin {
    Q_OBJECT
    Q_INTERFACES (FcitxQtConfigUIFactoryInterface)
public:
    explicit QuickPhraseEditorPlugin(QObject* parent = 0);
    virtual QString name();
    virtual QStringList files();
    virtual QString domain();
    virtual FcitxQtConfigUIWidget* create(const QString& key);
};
