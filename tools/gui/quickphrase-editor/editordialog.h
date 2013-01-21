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

#ifndef FCITX_TOOLS_GUI_DIALOG_H
#define FCITX_TOOLS_GUI_DIALOG_H

#include <QDialog>

class CMacroTable;
namespace Ui {
class EditorDialog;
}

namespace fcitx {
class EditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit EditorDialog(QWidget* parent = 0);
    virtual ~EditorDialog();

    QString key() const;
    QString value() const;
    void setValue(const QString& s);
    void setKey(const QString& s);

private:
    Ui::EditorDialog* m_ui;
};
}


#endif // FCITX_TOOLS_GUI_
