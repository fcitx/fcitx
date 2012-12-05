#include "wrapperapp.h"
#include "mainwindow.h"
#include "fcitx-qt/fcitxqtconfiguifactory.h"
#include "fcitx-qt/fcitxqtconnection.h"

WrapperApp::WrapperApp(int& argc, char** argv): QApplication(argc, argv)
    ,m_connection(new FcitxQtConnection(this))
{
    MainWindow* mainWindow = new MainWindow;
    mainWindow->show();
}

WrapperApp::~WrapperApp()
{

}
