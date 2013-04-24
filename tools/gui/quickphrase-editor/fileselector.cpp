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

#include <QMessageBox>
#include <QDebug>
#include <QFileInfoList>
#include <QInputDialog>
#include <cassert>

#include "fcitx-config/xdg.h"
#include "ui_fileselector.h"
#include "fileselector.h"

namespace fcitx{
    
FileSelector::FileSelector(const QDir& dir, QWidget* parent):m_ui(new Ui_FileSeletor)
{
    m_ui->setupUi(this);
    connect(m_ui->listWidget,SIGNAL(itemClicked(QListWidgetItem*)),this,SLOT(listWidgetClicked(QListWidgetItem*)));
    //connect(m_ui->listWidget,SIGNAL(itemActivated(QListWidgetItem*)),this,SLOT(listWidgetClicked(QListWidgetItem*)));
    connect(m_ui->editButton,SIGNAL(clicked(bool)),this,SLOT(editButtonClicked(bool)));
    connect(m_ui->addButton,SIGNAL(clicked(bool)),this,SLOT(addButtonClicked(bool)));
    connect(m_ui->deleteButton,SIGNAL(clicked(bool)),this,SLOT(deleteButtonClicked(bool)));
    connect(m_ui->refreshButton,SIGNAL(clicked(bool)),this,SLOT(refresh()));
    qDebug() << "showing file selector widget";
    m_dir = dir;
    refresh();
    show();
}

FileSelector::~FileSelector()
{
    //emit fileSelected("");
}

void FileSelector::listWidgetClicked(QListWidgetItem* item)
{
    selectedFilename = QDir("data/QuickPhrase.mb.d/").filePath(item->data(0).toString());
    qDebug() << selectedFilename;
}

void FileSelector::addButtonClicked(bool )
{
    QString filename = QInputDialog::getText(this
    ,tr("create new file")
    ,tr("Please input a filename for newfile")
    ,QLineEdit::Normal
    ,"newfile"
    );
    QFile file(m_dir.filePath(filename));
    file.open(QIODevice::ReadWrite);
    file.close();
    refresh();
}

void FileSelector::deleteButtonClicked(bool )
{
    QListWidgetItem* item = m_ui->listWidget->currentItem();
    int ret = QMessageBox::question(this
    ,tr("Confirm deleting")
    ,tr("Are you sure to delete this file :%1 ?").arg(item->data(0).toString())
    ,QMessageBox::Ok | QMessageBox::Cancel);
    qDebug() << ret;
    if (ret==QMessageBox::Ok){
        bool ret = m_dir.remove(item->data(0).toString());
        qDebug() << "file " << item->data(0).toString() << " deleted  " << ret;
    }
    refresh();
}

void FileSelector::editButtonClicked(bool )
{
    emit fileSelected(selectedFilename);
}

void FileSelector::refresh()
{
    m_ui->listWidget->clear();
    QFileInfoList list;
    list = m_dir.entryInfoList(QDir::Files | QDir::Readable | QDir::Writable);
    Q_FOREACH(QFileInfo i,list){
        m_ui->listWidget->addItem(i.fileName());
    }
}


void SelectorThread::run()
{
    QDir dir;
    //dir.setPath("data/QuickPhrase.mb.d/");
    size_t size;
    char** path = FcitxXDGGetPathUserWithPrefix(&size,"data/QuickPhrase.mb.d/");
    //dir.setPath(QDir::home().filePath(".config/fcitx/data/QuickPhrase.mb.d/"));
    assert(size>=1);
    dir.setPath(*path);
    if (!dir.exists()){
        emit finished("");
        qDebug() << "Dir " <<dir.absolutePath() <<  " does not exist";
        return ;
    }
    else{
        qDebug() << dir.path() << " found";
        selectorWidget = new FileSelector(dir);
        QEventLoop eventLoop;
        connect(selectorWidget,SIGNAL(fileSelected(QString)),this,SIGNAL(finished(QString)));
        connect(selectorWidget,SIGNAL(fileSelected(QString)),(&eventLoop),SLOT(quit()));
        eventLoop.exec();
    }
    //QThread::run();
}

SelectorThread::SelectorThread(QObject* parent): QThread(parent),fileAvailable(false),selectorWidget(NULL)
{
}

SelectorThread::~SelectorThread()
{
    if (selectorWidget!=NULL)
        delete selectorWidget;
}


};