#include <libintl.h>
#include <QPushButton>
#include <QDebug>

#include "mainwindow.h"
#include "fcitx-qt/fcitxqtconfiguifactory.h"
#include "fcitx-utils/utils.h"

MainWindow::MainWindow(QWidget* parent): QMainWindow(parent)
    ,m_ui(new Ui::MainWindow)
{
    FcitxQtConfigUIFactory* m_factory = new FcitxQtConfigUIFactory(this);
    m_pluginWidget = m_factory->create("data/QuickPhrase.mb");
    m_ui->setupUi(this);
    m_ui->verticalLayout->insertWidget(0, m_pluginWidget);
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(false);
    m_ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(false);
    setWindowIcon(QIcon::fromTheme("fcitx"));
    setWindowTitle(m_pluginWidget->title());

    qDebug() << connect(m_pluginWidget, SIGNAL(changed(bool)), this, SLOT(changed(bool)));
    connect(m_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(clicked(QAbstractButton*)));
}

void MainWindow::clicked(QAbstractButton* button)
{
    QDialogButtonBox::StandardButton standardButton = m_ui->buttonBox->standardButton(button);
    if (standardButton == QDialogButtonBox::Save)
        m_pluginWidget->save();
    else if (standardButton == QDialogButtonBox::Close) {
        qApp->quit();
    }
    else if (standardButton == QDialogButtonBox::Reset) {
        m_pluginWidget->load();
    }
}

void MainWindow::changed(bool changed)
{
    qDebug() << changed;
    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(changed);
    m_ui->buttonBox->button(QDialogButtonBox::Reset)->setEnabled(changed);
}

MainWindow::~MainWindow()
{
    delete m_ui;
}
