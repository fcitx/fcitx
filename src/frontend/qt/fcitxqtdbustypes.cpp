/*
 * Copyright (C) 2017~2020 by CSSlayer
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
