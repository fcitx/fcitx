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

#ifndef FCITX_FORMATTED_PREEDIT_H
#define FCITX_FORMATTED_PREEDIT_H

#include <fcitx/fcitx.h>

#include <QtCore/QMetaType>
#include <QtDBus/QDBusArgument>

class FCITX_EXPORT_API FcitxFormattedPreedit {
public:
    const QString& string() const;
    qint32 format() const;
    void setString(const QString& str);
    void setFormat(qint32 format);

    static void registerMetaType();

    bool operator ==(const FcitxFormattedPreedit& preedit) const;
private:
    QString m_string;
    qint32 m_format;
};

typedef QList<FcitxFormattedPreedit> FcitxFormattedPreeditList;

QDBusArgument& operator<<(QDBusArgument& argument, const FcitxFormattedPreedit& im);
const QDBusArgument& operator>>(const QDBusArgument& argument, FcitxFormattedPreedit& im);

Q_DECLARE_METATYPE(FcitxFormattedPreedit)
Q_DECLARE_METATYPE(FcitxFormattedPreeditList)

#endif
