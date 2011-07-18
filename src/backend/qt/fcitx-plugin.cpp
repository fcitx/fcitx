#include <QInputContextPlugin>
#include "fcitx-input-context.h"

#define FCITX_IDENTIFIER_NAME "fcitx"

/* The class Definition */
class FcitxPlugin: public QInputContextPlugin
{

private:
    /**
     * The language list for Fcitx.
     */
    static QStringList fcitx_languages;

public:

    FcitxPlugin (QObject *parent = 0);

    ~FcitxPlugin ();

    QStringList keys () const;

    QStringList languages (const QString &key);

    QString description (const QString &key);

    QInputContext *create (const QString &key);

    QString displayName (const QString &key);

private:

};


/* Implementations */
QStringList FcitxPlugin::fcitx_languages;


FcitxPlugin::FcitxPlugin (QObject *parent)
        :QInputContextPlugin (parent)
{
}


FcitxPlugin::~FcitxPlugin ()
{
}

QStringList
FcitxPlugin::keys () const
{
    QStringList identifiers;
    identifiers.push_back (FCITX_IDENTIFIER_NAME);
    return identifiers;
}


QStringList
FcitxPlugin::languages (const QString & key)
{
    if (key.toLower () != FCITX_IDENTIFIER_NAME) {
        return QStringList ();
    }

    if (fcitx_languages.empty ()) {
        fcitx_languages.push_back ("zh");
        fcitx_languages.push_back ("ja");
        fcitx_languages.push_back ("ko");
    }
    return fcitx_languages;
}


QString
FcitxPlugin::description (const QString &key)
{
    if (key.toLower () != FCITX_IDENTIFIER_NAME) {
        return QString ("");
    }

    return QString::fromUtf8 ("Qt immodule plugin for Fcitx");
}


QInputContext *
FcitxPlugin::create (const QString &key)
{
    if (key.toLower () != FCITX_IDENTIFIER_NAME) {
        return NULL;
    }

    return static_cast<QInputContext *> (new FcitxInputContext ());
}


QString FcitxPlugin::displayName (const QString &key)
{
    return key;
}

Q_EXPORT_PLUGIN2 (FcitxPlugin, FcitxPlugin)
