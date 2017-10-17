/*
 * Copyright (C) 2017~2017 by CSSlayer
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

#include <QDBusMetaType>

#include "fcitxqtdbustypes.h"

void FcitxFormattedPreedit::registerMetaType() {
    qRegisterMetaType<FcitxFormattedPreedit>("FcitxFormattedPreedit");
    qDBusRegisterMetaType<FcitxFormattedPreedit>();
    qRegisterMetaType<FcitxFormattedPreeditList>("FcitxFormattedPreeditList");
    qDBusRegisterMetaType<FcitxFormattedPreeditList>();
}

qint32 FcitxFormattedPreedit::format() const { return m_format; }

const QString &FcitxFormattedPreedit::string() const { return m_string; }

void FcitxFormattedPreedit::setFormat(qint32 format) { m_format = format; }

void FcitxFormattedPreedit::setString(const QString &str) { m_string = str; }

bool FcitxFormattedPreedit::
operator==(const FcitxFormattedPreedit &preedit) const {
    return (preedit.m_format == m_format) && (preedit.m_string == m_string);
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxFormattedPreedit &preedit) {
    argument.beginStructure();
    argument << preedit.string();
    argument << preedit.format();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxFormattedPreedit &preedit) {
    QString str;
    qint32 format;
    argument.beginStructure();
    argument >> str >> format;
    argument.endStructure();
    preedit.setString(str);
    preedit.setFormat(format);
    return argument;
}

void FcitxInputContextArgument::registerMetaType() {
    qRegisterMetaType<FcitxInputContextArgument>("FcitxInputContextArgument");
    qDBusRegisterMetaType<FcitxInputContextArgument>();
    qRegisterMetaType<FcitxInputContextArgumentList>(
        "FcitxInputContextArgumentList");
    qDBusRegisterMetaType<FcitxInputContextArgumentList>();
}

const QString &FcitxInputContextArgument::name() const { return m_name; }

void FcitxInputContextArgument::setName(const QString &name) { m_name = name; }

const QString &FcitxInputContextArgument::value() const { return m_value; }

void FcitxInputContextArgument::setValue(const QString &value) {
    m_value = value;
}

QDBusArgument &operator<<(QDBusArgument &argument,
                          const FcitxInputContextArgument &arg) {
    argument.beginStructure();
    argument << arg.name();
    argument << arg.value();
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument,
                                FcitxInputContextArgument &arg) {
    QString name, value;
    argument.beginStructure();
    argument >> name >> value;
    argument.endStructure();
    arg.setName(name);
    arg.setValue(value);
    return argument;
}
