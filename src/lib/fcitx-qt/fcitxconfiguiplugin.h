#ifndef FCITXCONFIGPLUGIN_H
#define FCITXCONFIGPLUGIN_H

#include "fcitxqt_export.h"
#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QStringList>

class FcitxConfigUIWidget;
struct FCITX_QT_EXPORT_API FcitxConfigUIFactoryInterface
{
    virtual QString name() = 0;
    virtual FcitxConfigUIWidget *create( const QString &key ) = 0;
    virtual QStringList files() = 0;

};

#define FcitxConfigUIFactoryInterface_iid "org.fcitx.Fcitx.FcitxConfigUIFactoryInterface"
Q_DECLARE_INTERFACE(FcitxConfigUIFactoryInterface, FcitxConfigUIFactoryInterface_iid)

class FCITX_QT_EXPORT_API FcitxConfigUIPlugin : public QObject, public FcitxConfigUIFactoryInterface {
    Q_OBJECT
    Q_INTERFACES(FcitxConfigUIFactoryInterface)
public:
    explicit FcitxConfigUIPlugin(QObject* parent = 0);
    virtual ~FcitxConfigUIPlugin();
};


#endif // FCITXCONFIGPLUGIN_H