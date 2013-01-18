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

#include <libintl.h>

#include <QDebug>
#include <QTimer>

#include "wrapperapp.h"
#include "mainwindow.h"
#include "fcitx-qt/fcitxqtconfiguifactory.h"
#include "fcitx-qt/fcitxqtconnection.h"
#include "fcitx-utils/utils.h"

WrapperApp::WrapperApp(int& argc, char** argv): QApplication(argc, argv)
    ,m_factory(new FcitxQtConfigUIFactory(this))
    ,m_mainWindow(0)
{
    char* localedir = fcitx_utils_get_fcitx_path("localedir");
    setlocale(LC_ALL, "");
    bindtextdomain("fcitx", localedir);
    free(localedir);
    bind_textdomain_codeset("fcitx", "UTF-8");
    textdomain("fcitx");

    FcitxQtConfigUIWidget* widget = 0;

    if (argc == 3 && strcmp(argv[1], "--test") == 0) {
        if (m_factory->test(QString::fromLocal8Bit(argv[2]))) {
            QTimer::singleShot(0, this, SLOT(quit()));
        } else {
            QTimer::singleShot(0, this, SLOT(errorExit()));
        }
    } else {
        if (argc == 2) {
            widget = m_factory->create(QString::fromLocal8Bit(argv[1]));
        }
        if (!widget) {
            qWarning("Could not find plugin for file.");
            QTimer::singleShot(0, this, SLOT(errorExit()));
        } else {
            m_mainWindow = new MainWindow(widget);
            m_mainWindow->show();
        }
    }
}

WrapperApp::~WrapperApp()
{
    if (m_mainWindow) {
        delete m_mainWindow;
    }
}

void WrapperApp::errorExit()
{
    exit(1);
}

