/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <QDBusMetaType>

#include "fcitxformattedpreedit.h"

void FcitxFormattedPreedit::registerMetaType()
{
    qRegisterMetaType<FcitxFormattedPreedit>("FcitxFormattedPreedit");
    qDBusRegisterMetaType<FcitxFormattedPreedit>();
    qRegisterMetaType<FcitxFormattedPreeditList>("FcitxFormattedPreeditList");
    qDBusRegisterMetaType<FcitxFormattedPreeditList>();
}

qint32 FcitxFormattedPreedit::format() const
{
    return m_format;
}

const QString& FcitxFormattedPreedit::string() const
{
    return m_string;
}

void FcitxFormattedPreedit::setFormat(qint32 format)
{
    m_format = format;
}

void FcitxFormattedPreedit::setString(const QString& str)
{
    m_string = str;
}

bool FcitxFormattedPreedit::operator==(const FcitxFormattedPreedit& preedit) const
{
    return (preedit.m_format == m_format) && (preedit.m_string == m_string);
}

QDBusArgument& operator<<(QDBusArgument& argument, const FcitxFormattedPreedit& preedit)
{
    argument.beginStructure();
    argument << preedit.string();
    argument << preedit.format();
    argument.endStructure();
    return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, FcitxFormattedPreedit& preedit)
{
    QString str;
    qint32 format;
    argument.beginStructure();
    argument >> str >> format;
    argument.endStructure();
    preedit.setString(str);
    preedit.setFormat(format);
    return argument;
}
