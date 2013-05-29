/***************************************************************************
 *   Copyright (C) 2013~2013 by CSSlayer                                   *
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

#include "filelistmodel.h"
#include "fcitx-config/xdg.h"

fcitx::FileListModel::FileListModel(QObject* parent): QAbstractListModel(parent)
{
    loadFileList();
}

fcitx::FileListModel::~FileListModel()
{

}

int fcitx::FileListModel::rowCount(const QModelIndex& parent) const
{
    return m_fileList.size();
}

QVariant fcitx::FileListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_fileList.size())
        return QVariant();

    switch (role) {
        case Qt::DisplayRole:
            return m_fileList[index.row()];
            //return m_fileList[index.row()].left(strlen("data/"));
        case Qt::UserRole:
            return m_fileList[index.row()];
        default:
            break;
    }
    return QVariant();
}

void fcitx::FileListModel::loadFileList()
{
    beginResetModel();
    m_fileList.clear();
    m_fileList.append(QUICK_PHRASE_CONFIG_FILE);
    FcitxStringHashSet* files = FcitxXDGGetFiles(QUICK_PHRASE_CONFIG_DIR, NULL, ".mb");
    HASH_FOREACH(f, files, FcitxStringHashSet) {
        m_fileList.append(QString::fromLocal8Bit(f->name).prepend(QUICK_PHRASE_CONFIG_DIR));
    }
    fcitx_utils_free_string_hash_set(files);

    qSort(m_fileList);

    endResetModel();
}

int fcitx::FileListModel::findFile(const QString& lastFileName)
{
    int idx = m_fileList.indexOf(lastFileName);
    if (idx < 0) {
        return 0;
    }
    return idx;
}