#include "fcitx-qt/fcitxconfiguiplugin.h"

class QuickPhraseEditorPlugin : public FcitxConfigUIPlugin {
    Q_OBJECT
    Q_INTERFACES (FcitxConfigUIFactoryInterface)
public:
    explicit QuickPhraseEditorPlugin(QObject* parent = 0);
    virtual QString name();
    virtual QStringList files();
    virtual FcitxConfigUIWidget* create(const QString& key);
};
