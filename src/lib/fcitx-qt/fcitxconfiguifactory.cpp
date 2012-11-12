#include "fcitxconfiguifactory.h"
#include "fcitxconfiguiplugin.h"
#include <fcitx-config/xdg.h>
#include <fcitx-utils/utils.h>
#include <QDir>
#include <QDebug>
#include <QLibrary>
#include <QPluginLoader>

FcitxConfigUIFactory::FcitxConfigUIFactory(QObject* parent): QObject(parent)
{
    scan();
}

FcitxConfigUIFactory::~FcitxConfigUIFactory()
{

}

FcitxConfigUIWidget* FcitxConfigUIFactory::create(const QString& file)
{
    return plugins[file]->create(file);
}

void FcitxConfigUIFactory::scan()
{
    QStringList dirs;
    // check plugin files
    size_t len;
    char** path = FcitxXDGGetLibPath(&len);
    for (int i = 0; i < len; i ++) {
        dirs << path[i];
    }

    if (dirs.isEmpty())
        return;
    for (QStringList::ConstIterator it = dirs.begin(); it != dirs.end(); ++it) {
        // qDebug() << QString ("Checking Qt Library Path: %1").arg (*it);
        QDir libpath (*it);
        QDir dir (libpath.filePath (QString ("qt")));
        if (!dir.exists()) {
            continue;
        }

        QStringList entryList = dir.entryList();
        // filter out "." and ".." to keep debug output cleaner
        entryList.removeAll (".");
        entryList.removeAll ("..");
        if (entryList.isEmpty()) {
            continue;
        }

        foreach (const QString & maybeFile, entryList) {
            QFileInfo fi (dir.filePath (maybeFile));

            QString filePath = fi.filePath(); // file name with path
            QString fileName = fi.fileName(); // just file name

            qDebug() << filePath;

            if (!QLibrary::isLibrary (filePath)) {
                continue;
            }

            QPluginLoader* loader = new QPluginLoader (filePath, this);
            qDebug() << loader->load();
            qDebug() << loader->errorString();
            FcitxConfigUIFactoryInterface* plugin = qobject_cast< FcitxConfigUIFactoryInterface* > (loader->instance());
            if (plugin) {
                QStringList list = plugin->files();
                foreach(const QString& s, list) {
                    plugins[s] = plugin;
                }
            }
        }
    }
}