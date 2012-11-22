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
#include "abstractmodel.h"

class QFile;
namespace fcitx {

class Parser;
class QuickPhraseModel : public AbstractItemEditorModel {
    Q_OBJECT
public:
    explicit QuickPhraseModel(QObject* parent = 0);
    virtual ~QuickPhraseModel();

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual void load();
    virtual void addItem(const QString& macro, const QString& word);
    virtual void deleteItem(int row);
    virtual void deleteAllItem();
    virtual void save();

private slots:
    void loadFinished();
    void saveFinished();

private:
    QSet<QString> m_keyset;
    QList<QPair< QString, QString > >m_list;
    Parser* m_parser;
};

}
