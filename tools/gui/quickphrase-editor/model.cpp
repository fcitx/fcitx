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

#include <QApplication>
#include <QThread>

#include "model.h"
#include "parser.h"
#include "common.h"
#include "editor.h"

namespace fcitx {

typedef QPair<QString, QString> ItemType;

QuickPhraseModel::QuickPhraseModel(QObject* parent): AbstractItemEditorModel(parent)
    ,m_parser(new Parser)
{
}

QuickPhraseModel::~QuickPhraseModel()
{

}

QVariant QuickPhraseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return _("Macro");
        else if (section == 1)
            return _("Word");
    }
    return QVariant();
}

int QuickPhraseModel::rowCount(const QModelIndex& parent) const
{
    return m_list.count();
}

int QuickPhraseModel::columnCount(const QModelIndex& parent) const
{
    return 2;
}

QVariant QuickPhraseModel::data(const QModelIndex& index, int role) const
{
    do {
        if (role == Qt::DisplayRole && index.row() < m_list.count()) {
            if (index.column() == 0) {
                return m_list[index.row()].first;
            } else if (index.column() == 1) {
                return m_list[index.row()].second;
            }
        }
    } while(0);
    return QVariant();
}

void QuickPhraseModel::addItem(const QString& macro, const QString& word)
{
    if (m_keyset.contains(macro))
        return;
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(QPair<QString, QString>(macro, word));
    m_keyset.insert(macro);
    endInsertRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteItem(int row)
{
    if (row >= m_list.count())
        return;
    QPair<QString, QString> item = m_list.at(row);
    QString key = item.first;
    beginRemoveRows(QModelIndex(), row, row);
    m_list.removeAt(row);
    m_keyset.remove(key);
    endRemoveRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteAllItem()
{
    if (m_list.count())
        setNeedSave(true);
    beginResetModel();
    m_list.clear();
    m_keyset.clear();
    endResetModel();
}

void QuickPhraseModel::load()
{
    QThread* thread = new QThread;
    m_parser->moveToThread(thread);
    connect(thread, SIGNAL(started()), m_parser, SLOT(run()));
    thread->start();
    connect(m_parser, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), this, SLOT(loadFinished()));
    //delete thread only when thread has really finished
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(thread, SIGNAL(terminated()), thread, SLOT(deleteLater()));
}

void QuickPhraseModel::loadFinished()
{
    m_parser->moveToThread(QThread::currentThread());
    m_list.clear();
    m_keyset.clear();
    beginResetModel();
    m_list = m_parser->m_list;
    m_keyset = m_parser->m_keyset;
    endResetModel();
}

void QuickPhraseModel::save()
{
}

void QuickPhraseModel::saveFinished()
{
    setNeedSave(false);
}


}