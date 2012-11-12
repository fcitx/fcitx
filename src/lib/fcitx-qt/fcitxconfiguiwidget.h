#ifndef FCITXCONFIGUIWIDGET_H
#define FCITXCONFIGUIWIDGET_H
#include "fcitxqt_export.h"
#include <QWidget>

class FCITX_QT_EXPORT_API FcitxConfigUIWidget : public QWidget {
    Q_OBJECT
public:
    explicit FcitxConfigUIWidget(QWidget* parent = 0);

    virtual void load(const QString& file) = 0;
    virtual void save() = 0;
};

#endif // FCITXCONFIGUIWIDGET_H