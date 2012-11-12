#include "mainwindow.h"
#include <fcitx-qt/fcitxconfiguifactory.h>

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent)
{
    FcitxConfigUIFactory* m_factory = new FcitxConfigUIFactory(this);
    setCentralWidget(m_factory->create("data/QuickPhrase.mb"));
}

MainWindow::~MainWindow()
{

}
