/***************************************************************************
 *   Copyright (C) 2012~2012 by CSSlayer                                   *
 *   wengxt@gmail.com                                                      *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                         *
 ***************************************************************************/

#include "common.h"
#include "ui_editordialog.h"
#include "editordialog.h"

namespace fcitx {
EditorDialog::EditorDialog(QWidget* parent): QDialog(parent),
    m_ui(new Ui::EditorDialog)
{
    m_ui->setupUi(this);
    m_ui->keyLabel->setText(_("Keyword:"));
    m_ui->valueLabel->setText(_("Phrase:"));
}

EditorDialog::~EditorDialog()
{
    delete m_ui;
}

void EditorDialog::setKey(const QString& s)
{
    m_ui->keyLineEdit->setText(s);
}

void EditorDialog::setValue(const QString& s)
{
    m_ui->valueLineEdit->setText(s);
}

QString EditorDialog::key() const
{
    return m_ui->keyLineEdit->text();
}

QString EditorDialog::value() const
{
    return m_ui->valueLineEdit->text();
}


}
