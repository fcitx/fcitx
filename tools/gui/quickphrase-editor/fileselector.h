/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) 2013  <copyright holder> <email>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef FCITX_TOOLS_GUI_FILESELECTOR_H
#define FCITX_TOOLS_GUI_FILESELECTOR_H

#include <QListWidget>
#include <QStringList>
#include <QThread>
#include <QDir>

#include "fcitx-qt/fcitxqtconfiguiwidget.h"

class Ui_FileSeletor;
namespace fcitx {
    
class ListEditor;

class FileSelector : public QWidget
{
    Q_OBJECT
public:
    explicit FileSelector(const QDir& dir, QWidget* parent = 0);
    virtual ~FileSelector();
public slots:
    void listWidgetClicked(QListWidgetItem*);
    void addButtonClicked(bool);
    void editButtonClicked(bool);
    void deleteButtonClicked(bool);
    void refresh();
signals:
    void fileSelected(QString);
protected:
    QString selectedFilename;
    QDir m_dir;
    Ui_FileSeletor* m_ui;
};

class SelectorThread : public QThread
{
    Q_OBJECT
public:
    explicit SelectorThread(QObject* parent = 0);
    virtual ~SelectorThread();
    void run();
signals:
    void finished(QString);
protected:
    bool fileAvailable;
    FileSelector* selectorWidget;
};

}
#endif // FILESELECTOR_H
