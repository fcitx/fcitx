#ifndef QCLOUD_FACTORY_H
#define QCLOUD_FACTORY_H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QStringList>

#include "fcitxqt_export.h"
#include "fcitxconfiguiwidget.h"
#include "fcitxconfiguiplugin.h"

class FcitxConfigUIFactoryPrivate;
class FCITX_QT_EXPORT_API FcitxConfigUIFactory : public QObject
{
    Q_OBJECT
public:
    explicit FcitxConfigUIFactory(QObject* parent = 0);
    virtual ~FcitxConfigUIFactory();
    FcitxConfigUIWidget* create(const QString& file);
private:
    FcitxConfigUIFactoryPrivate* d_ptr;
    QMap<QString, FcitxConfigUIFactoryInterface*> plugins;
    Q_DECLARE_PRIVATE(FcitxConfigUIFactory);
    void scan();
};

#endif // QCLOUD_FACTORY_H
