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

#ifndef FCITX_TOOLS_GUI_EDITOR_H
#define FCITX_TOOLS_GUI_EDITOR_H

#include <QMainWindow>
#include "fcitx-qt/fcitxqtconfiguiwidget.h"
#include "model.h"

class QAbstractItemModel;
class CMacroTable;
namespace Ui {
class Editor;
}

namespace fcitx {

class ListEditor : public FcitxQtConfigUIWidget {
    Q_OBJECT
public:
    explicit ListEditor(QuickPhraseModel* model, QWidget* parent = 0);
    virtual ~ListEditor();

    virtual void load();
    virtual void save();
    virtual QString title();
    virtual QString addon();
    virtual bool asyncSave();

private slots:
    void addWord();
    void batchEditWord();
    void deleteWord();
    void deleteAllWord();
    void itemFocusChanged();
    void addWordAccepted();
    void importData();
    void exportData();
    void importFileSelected();
    void exportFileSelected();
private:
    void load(const QString& file);
    void save(const QString& file);
    Ui::Editor* m_ui;
    QuickPhraseModel* m_model;
public slots:
    void batchEditAccepted();
};
}

#endif // FCITX_TOOLS_GUI_EDITOR_H
