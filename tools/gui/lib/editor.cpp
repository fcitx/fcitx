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
#include <fcitx-config/xdg.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>

#include "common.h"
#include "editor.h"
#include "dialog.h"
#include "abstractmodel.h"
#include "ui_editor.h"

namespace fcitx {

ListEditor::ListEditor(fcitx::AbstractItemEditorModel* model, QWidget* parent): FcitxConfigUIWidget(parent)
    ,m_ui(new Ui::Editor)
    ,m_model(model)
{
    m_ui->setupUi(this);
    m_ui->addButton->setText(_("&Add"));
    m_ui->deleteButton->setText(_("&Delete"));
    m_ui->clearButton->setText(_("De&lete All"));
    m_ui->exitButton->setText(_("&Quit"));
    m_ui->saveButton->setText(_("&Save"));
    m_ui->importButton->setText(_("&Import"));
    m_ui->exportButton->setText(_("&Export"));
    m_ui->macroTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->macroTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    setWindowTitle(_("Unikey Macro Editor"));

    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addWord()));
    connect(m_ui->deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteWord()));
    connect(m_ui->clearButton, SIGNAL(clicked(bool)), this, SLOT(deleteAllWord()));
    connect(m_ui->importButton, SIGNAL(clicked(bool)), this, SLOT(importMacro()));
    connect(m_ui->exportButton, SIGNAL(clicked(bool)), this, SLOT(exportMacro()));
    connect(m_ui->exitButton, SIGNAL(clicked(bool)), this, SLOT(aboutToQuit()));
    connect(m_ui->saveButton, SIGNAL(clicked(bool)), this, SLOT(saveMacro()));

    load();
    itemFocusChanged();
}

void ListEditor::load(const QString& file)
{

}

void ListEditor::save()
{

}


ListEditor::~ListEditor()
{
    delete m_ui;
}

void ListEditor::aboutToQuit()
{
    if (!m_model->needSave())
        qApp->quit();
    else {
        QMessageBox* dialog = new QMessageBox(this);
        dialog->setIcon(QMessageBox::Warning);
        dialog->setWindowTitle(_("Quit Macro Editor"));
        dialog->setText(_("Macro table still contains unsaved changes. Do you want to save?"));
        dialog->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        dialog->setDefaultButton(QMessageBox::Save);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
        connect(dialog, SIGNAL(finished(int)), this, SLOT(quitConfirmDone(int)));
    }
}

void ListEditor::quitConfirmDone(int result)
{
    switch(result) {
        case QMessageBox::Save:
            saveMacro();
        case QMessageBox::Discard:
            qApp->quit();
            break;
    }
}

void ListEditor::closeEvent(QCloseEvent* event)
{
    if (m_model->needSave()) {
        event->ignore();

        QMessageBox* dialog = new QMessageBox(this);
        dialog->setIcon(QMessageBox::Warning);
        dialog->setWindowTitle(_("Quit Macro Editor"));
        dialog->setText(_("Macro table still contains unsaved changes. Do you want to save?"));
        dialog->setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        dialog->setDefaultButton(QMessageBox::Save);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->open();
        connect(dialog, SIGNAL(finished(int)), this, SLOT(quitConfirmDone(int)));
    }
    else {
        event->accept();
        qApp->quit();
    }
}


void ListEditor::itemFocusChanged()
{
    m_ui->deleteButton->setEnabled(m_ui->macroTableView->currentIndex().isValid());
}

void ListEditor::deleteWord()
{
    if (!m_ui->macroTableView->currentIndex().isValid())
        return;
    int row = m_ui->macroTableView->currentIndex().row();
    m_model->deleteItem(row);
}

void ListEditor::deleteAllWord()
{
    m_model->deleteAllItem();
}

void ListEditor::addWord()
{
    MacroDialog* dialog = new MacroDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(addWordAccepted()));
}

void ListEditor::addWordAccepted()
{
     const MacroDialog* dialog = qobject_cast< const MacroDialog* >(QObject::sender());

     m_model->addItem(dialog->macro(), dialog->word());
}

void ListEditor::load()
{
    m_ui->macroTableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->macroTableView->verticalHeader()->setVisible(false);
    m_ui->macroTableView->setModel(m_model);
    connect(m_ui->macroTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(itemFocusChanged()));
    connect(m_model, SIGNAL(needSaveChanged(bool)), this, SLOT(needSaveChanged(bool)));
    m_model->load();
}

void ListEditor::needSaveChanged(bool needSave)
{
    m_ui->saveButton->setEnabled(needSave);
}


void ListEditor::saveMacro()
{
    m_model->save();
}

void ListEditor::importMacro()
{
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(importFileSelected()));
}

void ListEditor::importFileSelected()
{
     const QFileDialog* dialog = qobject_cast< const QFileDialog* >(QObject::sender());
     qDebug() << dialog->selectedFiles();
}

void ListEditor::exportMacro()
{
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setDirectory("macro");
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(exportFileSelected()));
}

void ListEditor::exportFileSelected()
{
     const QFileDialog* dialog = qobject_cast< const QFileDialog* >(QObject::sender());
     if (dialog->selectedFiles().length() <= 0)
         return;
     QString file = dialog->selectedFiles()[0];
}




}