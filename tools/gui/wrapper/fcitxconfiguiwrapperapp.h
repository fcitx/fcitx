#ifndef FCITXCONFIGUIWRAPPERAPP_H
#define FCITXCONFIGUIWRAPPERAPP_H

#include <QtGui/QApplication>

class FcitxConfigUIFactory;
class FcitxConfigUIWrapperApp : public QApplication {
    Q_OBJECT
    FcitxConfigUIFactory* m_factory;
public:
    FcitxConfigUIWrapperApp(int& argc, char** argv);
    virtual ~FcitxConfigUIWrapperApp();
};

#endif // FCITXCONFIGUIWRAPPERAPP_H
