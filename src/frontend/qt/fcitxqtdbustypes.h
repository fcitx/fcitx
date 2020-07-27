/*
 * Copyright (C) 2012~2020 by CSSlayer
 * wengxt@gmail.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above Copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above Copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 */
#ifndef _DBUSADDONS_FCITXQTDBUSTYPES_H_
#define _DBUSADDONS_FCITXQTDBUSTYPES_H_

#include <QDBusArgument>
#include <QList>
#include <QMetaType>

class FcitxFormattedPreedit {
public:
    const QString &string() const;
    qint32 format() const;
    void setString(const QString &str);
    void setFormat(qint32 format);

    static void registerMetaType();

    bool operator==(const FcitxFormattedPreedit &preedit) const;

private:
    QString m_string;
    qint32 m_format = 0;
};

typedef QList<FcitxFormattedPreedit> FcitxFormattedPreeditList;

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxFormattedPreedit &im);
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxFormattedPreedit &im);

class FcitxInputContextArgument {
public:
    FcitxInputContextArgument() {}
    FcitxInputContextArgument(const QString &name, const QString &value)
        : m_name(name), m_value(value) {}

    static void registerMetaType();

    const QString &name() const;
    const QString &value() const;
    void setName(const QString &);
    void setValue(const QString &);

private:
    QString m_name;
    QString m_value;
};

typedef QList<FcitxInputContextArgument> FcitxInputContextArgumentList;

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxInputContextArgument &im);
const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxInputContextArgument &im);

Q_DECLARE_METATYPE(FcitxFormattedPreedit)
Q_DECLARE_METATYPE(FcitxFormattedPreeditList)

Q_DECLARE_METATYPE(FcitxInputContextArgument)
Q_DECLARE_METATYPE(FcitxInputContextArgumentList)

#endif // _DBUSADDONS_FCITXQTDBUSTYPES_H_
