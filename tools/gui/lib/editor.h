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

#include <QMainWindow>
#include <fcitx-qt/fcitxconfiguiwidget.h>

class QAbstractItemModel;
class CMacroTable;
namespace Ui {
class Editor;
}

namespace fcitx {

class AbstractItemEditorModel;

class ListEditor : public FcitxConfigUIWidget {
    Q_OBJECT
public:
    explicit ListEditor(AbstractItemEditorModel* model, QWidget* parent = 0);
    virtual ~ListEditor();

    virtual void load(const QString& file);
    virtual void save();
protected:
    virtual void closeEvent(QCloseEvent* event );
private slots:
    void addWord();
    void deleteWord();
    void deleteAllWord();
    void itemFocusChanged();
    void aboutToQuit();
    void saveMacro();
    void load();
    void addWordAccepted();
    void importMacro();
    void exportMacro();
    void importFileSelected();
    void exportFileSelected();
    void needSaveChanged(bool needSave);
    void quitConfirmDone(int result);
private:
    Ui::Editor* m_ui;
    AbstractItemEditorModel* m_model;
};
}
