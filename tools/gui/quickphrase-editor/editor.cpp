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
#include <QInputDialog>
#include <QMenu>
#include <qtconcurrentrun.h>
#include <cassert>
#include <cstdio>

#include "common.h"
#include "editor.h"
#include "editordialog.h"
#include "model.h"
#include "batchdialog.h"
#include "filelistmodel.h"
#include "ui_editor.h"

namespace fcitx
{

ListEditor::ListEditor(QWidget* parent)
    : FcitxQtConfigUIWidget(parent),
      m_ui(new Ui::Editor),
      m_model(new QuickPhraseModel(this)),
      m_fileListModel(new FileListModel(this))
{
    m_ui->setupUi(this);
    m_ui->addButton->setText(_("&Add"));
    m_ui->batchEditButton->setText(_("&Batch Edit"));
    m_ui->deleteButton->setText(_("&Delete"));
    m_ui->clearButton->setText(_("De&lete All"));
    m_ui->importButton->setText(_("&Import"));
    m_ui->exportButton->setText(_("E&xport"));
    m_ui->operationButton->setText(_("&Operation"));
    m_ui->macroTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_ui->macroTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_ui->macroTableView->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_ui->macroTableView->horizontalHeader()->setStretchLastSection(true);
    m_ui->macroTableView->verticalHeader()->setVisible(false);
    m_ui->macroTableView->setModel(m_model);
    m_ui->fileListComboBox->setModel(m_fileListModel);

    m_operationMenu = new QMenu(this);
    m_operationMenu->addAction(_("Add File"), this, SLOT(addFileTriggered()));
    m_operationMenu->addAction(_("Remove File"), this, SLOT(removeFileTriggered()));
    m_operationMenu->addAction(_("Refresh List"), this, SLOT(refreshListTriggered()));
    m_ui->operationButton->setMenu(m_operationMenu);

    loadFileList();
    itemFocusChanged();

    connect(m_ui->addButton, SIGNAL(clicked(bool)), this, SLOT(addWord()));
    connect(m_ui->batchEditButton, SIGNAL(clicked(bool)), this, SLOT(batchEditWord()));
    connect(m_ui->deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteWord()));
    connect(m_ui->clearButton, SIGNAL(clicked(bool)), this, SLOT(deleteAllWord()));
    connect(m_ui->importButton, SIGNAL(clicked(bool)), this, SLOT(importData()));
    connect(m_ui->exportButton, SIGNAL(clicked(bool)), this, SLOT(exportData()));

    connect(m_ui->fileListComboBox, SIGNAL(activated(int)), this, SLOT(changeFile(int)));

    connect(m_ui->macroTableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(itemFocusChanged()));
    connect(m_model, SIGNAL(needSaveChanged(bool)), this, SIGNAL(changed(bool)));
}

ListEditor::~ListEditor()
{
    delete m_ui;
}

void ListEditor::load()
{
    m_lastFile = currentFile();
    m_model->load(currentFile(), false);
}

void ListEditor::load(const QString& file)
{
    m_model->load(file, true);
}

void ListEditor::save(const QString& file)
{
    m_model->save(file);
}

void ListEditor::save()
{
    //QFutureWatcher< bool >* futureWatcher = m_model->save("data/QuickPhrase.mb");
    QFutureWatcher< bool >* futureWatcher = m_model->save(currentFile());
    connect(futureWatcher, SIGNAL(finished()), this, SIGNAL(saveFinished()));
}

QString ListEditor::addon()
{
    return "fcitx-quickphrase";
}

bool ListEditor::asyncSave()
{
    return true;
}

void ListEditor::changeFile(int newIndex)
{
    if (m_model->needSave()) {
        int ret = QMessageBox::question(this,
                                        _("Save Changes"),
                                        _("The content has changed.\n"
                                          "Do you want to save the changes or discard them?"),
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        if (ret == QMessageBox::Save) {
            //save(m_ui->fileListComboBox->itemText(lastFileIndex));
            save(m_lastFile);
        } else if (ret == QMessageBox::Cancel) {
            m_ui->fileListComboBox->setCurrentIndex(m_fileListModel->findFile(m_lastFile));
            return;
        }
    }
    load();
}

QString ListEditor::title()
{
    return _("Quick Phrase Editor");
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
    EditorDialog* dialog = new EditorDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(addWordAccepted()));
}

void ListEditor::batchEditWord()
{
    BatchDialog* dialog = new BatchDialog(this);
    QString text;
    QTextStream stream(&text);
    m_model->saveData(stream);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setText(text);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(batchEditAccepted()));
}

void ListEditor::addWordAccepted()
{
    const EditorDialog* dialog = qobject_cast< const EditorDialog* >(QObject::sender());

    m_model->addItem(dialog->key(), dialog->value());
    QModelIndex last = m_model->index(m_model->rowCount() - 1, 0);
    m_ui->macroTableView->setCurrentIndex(last);
    m_ui->macroTableView->scrollTo(last);
}

void ListEditor::batchEditAccepted()
{
    const BatchDialog* dialog = qobject_cast< const BatchDialog* >(QObject::sender());

    QString s = dialog->text();
    QTextStream stream(&s);

    m_model->loadData(stream);
    QModelIndex last = m_model->index(m_model->rowCount() - 1, 0);
    m_ui->macroTableView->setCurrentIndex(last);
    m_ui->macroTableView->scrollTo(last);
}

void ListEditor::importData()
{
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setFileMode(QFileDialog::ExistingFile);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(importFileSelected()));
}

void ListEditor::exportData()
{
    QFileDialog* dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->open();
    connect(dialog, SIGNAL(accepted()), this, SLOT(exportFileSelected()));
}


void ListEditor::importFileSelected()
{
    const QFileDialog* dialog = qobject_cast< const QFileDialog* >(QObject::sender());
    if (dialog->selectedFiles().length() <= 0)
        return;
    QString file = dialog->selectedFiles()[0];
    load(file);
}

void ListEditor::exportFileSelected()
{
    const QFileDialog* dialog = qobject_cast< const QFileDialog* >(QObject::sender());
    if (dialog->selectedFiles().length() <= 0)
        return;
    QString file = dialog->selectedFiles()[0];
    save(file);
}

void ListEditor::loadFileList()
{
    int row = m_ui->fileListComboBox->currentIndex();
    int col = m_ui->fileListComboBox->modelColumn();
    QString lastFileName = m_fileListModel->data(m_fileListModel->index(row, col), Qt::UserRole).toString();
    m_fileListModel->loadFileList();
    m_ui->fileListComboBox->setCurrentIndex(m_fileListModel->findFile(lastFileName));
    load();
}

QString ListEditor::currentFile()
{
    int row = m_ui->fileListComboBox->currentIndex();
    int col = m_ui->fileListComboBox->modelColumn();
    return m_fileListModel->data(m_fileListModel->index(row, col), Qt::UserRole).toString();
}

QString ListEditor::currentName()
{
    int row = m_ui->fileListComboBox->currentIndex();
    int col = m_ui->fileListComboBox->modelColumn();
    return m_fileListModel->data(m_fileListModel->index(row, col), Qt::DisplayRole).toString();
}

void ListEditor::addFileTriggered()
{
    bool ok;
    QString filename = QInputDialog::getText(this,
                       _("Create new file"),
                       _("Please input a filename for newfile"),
                       QLineEdit::Normal, "newfile", &ok
                                            );

    if (filename.contains('/')) {
        QMessageBox::warning(this, _("Invalid filename"), _("File name should not contain '/'."));
        return;
    }

    std::FILE* file = FcitxXDGGetFileUserWithPrefix(QUICK_PHRASE_CONFIG_DIR, filename.append(".mb").toLocal8Bit(), "w", NULL);

    if (file) {
        fclose(file);
    } else {
        QMessageBox::warning(this,
                             _("File Operation Failed"),
                             _("Cannot create file %1.").arg(filename)
                            );
        return;
    }

    m_fileListModel->loadFileList();
    m_ui->fileListComboBox->setCurrentIndex(m_fileListModel->findFile(filename.prepend(QUICK_PHRASE_CONFIG_DIR "/")));
    load();

}

void ListEditor::refreshListTriggered()
{
    loadFileList();
}

void ListEditor::removeFileTriggered()
{
    QString filename = currentFile();
    QString curName = currentName();
    char* fullname = NULL;
    FcitxXDGGetFileUserWithPrefix(NULL, filename.toLocal8Bit(), NULL, &fullname);
    QFile f(fullname);
    free(fullname);
    if (!f.exists()) {
        int ret = QMessageBox::question(this,
                                        _("Cannot remove system file"),
                                        _("%1 is a system file, do you want to delete all phrases instead?").arg(curName),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::Yes
                                       );
        if (ret == QMessageBox::Yes) {
            deleteAllWord();
        }
        return;
    }

    int ret = QMessageBox::question(this,
                                    _("Confirm deletion"),
                                    _("Are you sure to delete %1?").arg(curName),
                                    QMessageBox::Ok | QMessageBox::Cancel);

    if (ret == QMessageBox::Ok) {
        bool ok = f.remove();
        if (!ok) {
            QMessageBox::warning(this,
                                 _("File Operation Failed"),
                                 _("Error while deleting %1.").arg(curName)
                                );
        }
    }
    loadFileList();
    load();
}

}
