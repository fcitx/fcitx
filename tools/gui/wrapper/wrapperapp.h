#ifndef FCITXCONFIGUIWRAPPERAPP_H
#define FCITXCONFIGUIWRAPPERAPP_H

#include <QApplication>

class FcitxQtConnection;
class FcitxQtConfigUIFactory;
class WrapperApp : public QApplication {
    Q_OBJECT
    FcitxQtConfigUIFactory* m_factory;
public:
    WrapperApp(int& argc, char** argv);
    virtual ~WrapperApp();
private:
    FcitxQtConnection* m_connection;
};

#endif // FCITXCONFIGUIWRAPPERAPP_H
