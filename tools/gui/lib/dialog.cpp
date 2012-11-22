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

#include "ui_dialog.h"
#include "dialog.h"

namespace fcitx {
MacroDialog::MacroDialog(QWidget* parent): QDialog(parent),
    m_ui(new Ui::Dialog)
{
    m_ui->setupUi(this);
}

MacroDialog::~MacroDialog()
{
    delete m_ui;
}

QString MacroDialog::macro() const
{
    return m_ui->macroLineEdit->text();
}

QString MacroDialog::word() const
{
    return m_ui->wordLineEdit->text();
}


}
