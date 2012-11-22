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

#include <QAbstractItemModel>
#include <QSet>

class QFile;
namespace fcitx {
class AbstractItemEditorModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit AbstractItemEditorModel(QObject* parent = 0);
    virtual ~AbstractItemEditorModel();

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const = 0;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const = 0;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const = 0;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const = 0;
    virtual void load() = 0;
    virtual void addItem(const QString& macro, const QString& word) = 0;
    virtual void deleteItem(int row) = 0;
    virtual void deleteAllItem() = 0;
    virtual void save() = 0;
    bool needSave();

signals:
    void needSaveChanged(bool m_needSave);

protected:
    void setNeedSave(bool needSave);
    bool m_needSave;
};

}
