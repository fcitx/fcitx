#include <QMainWindow>

#include "ui_mainwindow.h"
#include "fcitx-qt/fcitxqtconfiguiwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = 0);
    virtual ~MainWindow();
public slots:
    void changed(bool changed);
    void clicked(QAbstractButton* button);

private:
    Ui::MainWindow* m_ui;
    FcitxQtConfigUIWidget* m_pluginWidget;
};
