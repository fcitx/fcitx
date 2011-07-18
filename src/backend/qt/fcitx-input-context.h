#ifndef __FCITX_INPUT_CONTEXT_H_
#define __FCITX_INPUT_CONTEXT_H_
#include <QInputContext>
#include <QList>

class FcitxInputContext : public QInputContext {
    Q_OBJECT
public:
    FcitxInputContext ();
    ~FcitxInputContext ();

    virtual QString identifierName();
    virtual QString language();
    virtual void reset();
    virtual bool isComposing() const;
};

#endif //__FCITX_INPUT_CONTEXT_H_
