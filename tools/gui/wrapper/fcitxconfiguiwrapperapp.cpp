#include "fcitxconfiguiwrapperapp.h"
#include "mainwindow.h"
#include "fcitx-qt/fcitxconfiguifactory.h"

FcitxConfigUIWrapperApp::FcitxConfigUIWrapperApp(int& argc, char** argv): QApplication(argc, argv)
{
    MainWindow* mainWindow = new MainWindow;
    mainWindow->show();
}

FcitxConfigUIWrapperApp::~FcitxConfigUIWrapperApp()
{

}
