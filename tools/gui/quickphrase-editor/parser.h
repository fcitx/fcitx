#ifndef PARSER_H
#define PARSER_H

#include <QObject>
#include <QSet>
namespace fcitx {
class Parser : public QObject {
Q_OBJECT
public:
    explicit Parser(QObject* parent = 0);
    virtual ~Parser();


public slots:
    void run();

signals:
    void finished();

public:
    QSet<QString> m_keyset;
    QList<QPair< QString, QString > >m_list;
};
}

#endif