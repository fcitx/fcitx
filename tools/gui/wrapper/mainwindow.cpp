/***************************************************************************
 *   Copyright (C) 2012~2013 by CSSlayer                                   *
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

#include <libintl.h>
#include <QPushButton>
#include <QLocale>
#include <QDebug>

#include "common.h"
#include "mainwindow.h"
#include "fcitx-qt/fcitxqtconfiguifactory.h"
#include "fcitx-qt/fcitxqtconnection.h"
#include "fcitx-qt/fcitxqtinputmethodproxy.h"
#include "fcitx-utils/utils.h"

MainWindow::MainWindow(FcitxQtConfigUIWidget* pluginWidget, QWidget* parent): QMainWindow(parent)
    ,m_ui(new Ui::MainWindow)
    ,m_connection(new FcitxQtConnection(this))
    ,m_pluginWidget(pluginWidget)
    ,m_proxy(0)
{
    m_ui->setupUi(this);
    m_ui->verticalLayout->insertWidget(0, m_pluginWidget);
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setText(_("&Save"));
    m_ui->buttonBox->button(QDialogButtonBox::Reset)->setText(_("&Reset"));
    m_ui->buttonBox->button(QDialogButtonBox::Close)->setText(_("&Close"));
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    m_ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
    setWindowIcon(QIcon::fromTheme("fcitx"));
    setWindowTitle(m_pluginWidget->title());

    connect(m_pluginWidget, SIGNAL(changed(bool)), this, SLOT(changed(bool)));
    if (m_pluginWidget->asyncSave())
        connect(m_pluginWidget, SIGNAL(saveFinished()), this, SLOT(saveFinished()));
    connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
    connect(m_connection, SIGNAL(connected()), this, SLOT(connected()));

    m_connection->startConnection();
}

void MainWindow::connected()
{
    if (m_proxy) {
        delete m_proxy;
    }
    m_proxy = new FcitxQtInputMethodProxy(m_connection->serviceName(), QLatin1String("/inputmethod"), *m_connection->connection(), this);
}

void MainWindow::clicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton standardButton = m_ui->buttonBox->standardButton(button);
    if (standardButton == QDialogButtonBox::Save) {
        if (m_pluginWidget->asyncSave())
            m_pluginWidget->setEnabled(false);
        m_pluginWidget->save();
        if (!m_pluginWidget->asyncSave())
            saveFinished();
    } else if (standardButton == QDialogButtonBox::Close) {
        qApp->quit();
    }
    else if (standardButton == QDialogButtonBox::Reset) {
        m_pluginWidget->load();
    }
}

void MainWindow::saveFinished()
{
    if (m_pluginWidget->asyncSave())
        m_pluginWidget->setEnabled(true);
    if (m_proxy) {
        m_proxy->ReloadAddonConfig(m_pluginWidget->addon());
    }
}

void MainWindow::changed(bool changed)
{
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(changed);
    m_ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(changed);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}
