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
#include <qplugin.h>
#include <libintl.h>
#include <fcitx-utils/utils.h>
#include "main.h"
#include "editor.h"
#include "model.h"

QuickPhraseEditorPlugin::QuickPhraseEditorPlugin(QObject* parent): FcitxQtConfigUIPlugin(parent)
{

}

FcitxQtConfigUIWidget* QuickPhraseEditorPlugin::create(const QString& key)
{
    Q_UNUSED(key);
    return new fcitx::ListEditor(new fcitx::QuickPhraseModel);
}

QStringList QuickPhraseEditorPlugin::files()
{
    return QStringList("data/QuickPhrase.mb");
}

QString QuickPhraseEditorPlugin::name()
{
    return "quickphrase-editor";
}

QString QuickPhraseEditorPlugin::domain()
{
    return "fcitx";
}


Q_EXPORT_PLUGIN2 (fcitx_quickphrase_editor, QuickPhraseEditorPlugin)
