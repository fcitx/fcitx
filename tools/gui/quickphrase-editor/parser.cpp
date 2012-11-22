#include "parser.h"
#include <QFile>
#include "fcitx-config/xdg.h"
#include <QDebug>

namespace fcitx {
Parser::Parser(QObject* parent): QObject(parent)
{

}

Parser::~Parser()
{

}

void Parser::run()
{
    FILE* fp = FcitxXDGGetFileWithPrefix("data", "QuickPhrase.mb", "r", NULL);
    if (!fp)
        return;

    QFile file;
    if (!file.open(fp, QFile::ReadOnly)) {
        fclose(fp);
        return;
    }
    QByteArray line;
    while (!(line = file.readLine()).isNull()) {
        QString s = QString::fromUtf8(line);
        s = s.simplified();
        if (s.isEmpty())
            continue;
        QString key = s.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString value = s.section(" ", 1, -1, QString::SectionSkipEmpty);
        if (key.isEmpty() || value.isEmpty())
            continue;
        if (m_keyset.contains(key))
            continue;
        m_keyset.insert(key);
        m_list.append(QPair<QString, QString>(key, value));
    }

    file.close();
    fclose(fp);
    emit finished();
}

}
