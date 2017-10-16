/*
 * Copyright (C) 2012~2017 by CSSlayer
 * wengxt@gmail.com
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; see the file COPYING. If not,
 * see <http://www.gnu.org/licenses/>.
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
