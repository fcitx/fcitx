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
#include <QtConcurrentRun>
#include <QFile>
#include <QFutureWatcher>
#include <qtemporaryfile.h>

#include "model.h"
#include "common.h"
#include "editor.h"
#include <fcitx-config/xdg.h>

namespace fcitx {

typedef QPair<QString, QString> ItemType;

QuickPhraseModel::QuickPhraseModel(QObject* parent): QAbstractTableModel(parent)
{
}

QuickPhraseModel::~QuickPhraseModel()
{
}

bool QuickPhraseModel::needSave()
{
    return m_needSave;
}

QVariant QuickPhraseModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return _("Keyword");
        else if (section == 1)
            return _("Phrase");
    }
    return QVariant();
}

int QuickPhraseModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return m_list.count();
}

int QuickPhraseModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent);
    return 2;
}

QVariant QuickPhraseModel::data(const QModelIndex& index, int role) const
{
    do {
        if ((role == Qt::DisplayRole || role == Qt::EditRole) && index.row() < m_list.count()) {
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
    beginInsertRows(QModelIndex(), m_list.size(), m_list.size());
    m_list.append(QPair<QString, QString>(macro, word));
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
    endRemoveRows();
    setNeedSave(true);
}

void QuickPhraseModel::deleteAllItem()
{
    if (m_list.count())
        setNeedSave(true);
    beginResetModel();
    m_list.clear();
    endResetModel();
}

Qt::ItemFlags QuickPhraseModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool QuickPhraseModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (role != Qt::EditRole)
        return false;

    if (index.column() == 0) {
        m_list[index.row()].first = value.toString();

        emit dataChanged(index, index);
        setNeedSave(true);
        return true;
    } else if (index.column() == 1) {
        m_list[index.row()].second = value.toString();

        emit dataChanged(index, index);
        setNeedSave(true);
        return true;
    } else
        return false;
}

void QuickPhraseModel::load(const QString& file, bool append)
{
    beginResetModel();
    if (!append) {
        m_list.clear();
        setNeedSave(false);
    }
    else
        setNeedSave(true);
    QFutureWatcher< void >* futureWatcher = new QFutureWatcher< void >(this);
    futureWatcher->setFuture(QtConcurrent::run<void>(this, &QuickPhraseModel::parse, file));
    connect(futureWatcher, SIGNAL(finished()), this, SLOT(loadFinished()));
    connect(futureWatcher, SIGNAL(finished()), futureWatcher, SLOT(deleteLater()));
}

void QuickPhraseModel::parse(const QString& filename) {
    FILE* fp = FcitxXDGGetFileWithPrefix("", filename.toLocal8Bit().constData(), "r", NULL);
    if (!fp)
        return;

    QFile file;
    if (!file.open(fp, QFile::ReadOnly)) {
        fclose(fp);
        return;
    }
    QByteArray line;
    while (!(line = file.readLine()).isNull()) {
        QString s = QString::fromUtf8(line);
        s = s.simplified();
        if (s.isEmpty())
            continue;
        QString key = s.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString value = s.section(" ", 1, -1, QString::SectionSkipEmpty);
        if (key.isEmpty() || value.isEmpty())
            continue;
        m_list.append(QPair<QString, QString>(key, value));
    }

    file.close();
    fclose(fp);
}

void QuickPhraseModel::loadFinished()
{
    endResetModel();
}

QFutureWatcher< bool >* QuickPhraseModel::save(const QString& file)
{
    QFutureWatcher< bool >* futureWatcher = new QFutureWatcher< bool >(this);
    futureWatcher->setFuture(QtConcurrent::run<bool>(this, &QuickPhraseModel::saveData, file));
    connect(futureWatcher, SIGNAL(finished()), this, SLOT(saveFinished()));
    connect(futureWatcher, SIGNAL(finished()), futureWatcher, SLOT(deleteLater()));
    return futureWatcher;
}

void QuickPhraseModel::saveData(QTextStream& dev)
{
    for (int i = 0; i < m_list.size(); i ++) {
        dev << m_list[i].first << "\t" << m_list[i].second << "\n";
    }
}

void QuickPhraseModel::loadData(QTextStream& stream)
{
    beginResetModel();
    m_list.clear();
    setNeedSave(true);
    QString s;
    while (!(s = stream.readLine()).isNull()) {
        s = s.simplified();
        if (s.isEmpty())
            continue;
        QString key = s.section(" ", 0, 0, QString::SectionSkipEmpty);
        QString value = s.section(" ", 1, -1, QString::SectionSkipEmpty);
        if (key.isEmpty() || value.isEmpty())
            continue;
        m_list.append(QPair<QString, QString>(key, value));
    }
    endResetModel();
}

bool QuickPhraseModel::saveData(const QString& file)
{
    char* name = NULL;
    FcitxXDGGetFileWithPrefix("", file.toLocal8Bit().constData(), NULL, &name);
    QString fileName = QString::fromLocal8Bit(name);
    QTemporaryFile tempFile(fileName);
    free(name);
    if (!tempFile.open())
        return false;

    for (int i = 0; i < m_list.size(); i ++) {
        tempFile.write(m_list[i].first.toUtf8());
        tempFile.write("\t");
        tempFile.write(m_list[i].second.toUtf8());
        tempFile.write("\n");
    }

    tempFile.setAutoRemove(false);
    QFile::remove(fileName);
    if (!tempFile.rename(fileName))
        tempFile.remove();

    return true;
}

void QuickPhraseModel::saveFinished()
{
    QFutureWatcher< bool >* watcher = static_cast<QFutureWatcher<bool>*>( sender());
    QFuture< bool > future = watcher->future();
    if (future.result()) {
        setNeedSave(false);
    }
}

void QuickPhraseModel::setNeedSave(bool needSave)
{
    if (m_needSave != needSave) {
        m_needSave = needSave;
        emit needSaveChanged(m_needSave);
    }
}

}
