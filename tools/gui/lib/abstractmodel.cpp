#include "abstractmodel.h"

namespace fcitx {

AbstractItemEditorModel::AbstractItemEditorModel(QObject* parent): QAbstractTableModel(parent)
    ,m_needSave(false)
{

}

AbstractItemEditorModel::~AbstractItemEditorModel()
{

}


void AbstractItemEditorModel::setNeedSave(bool needSave)
{
    if (m_needSave != needSave) {
        m_needSave = needSave;
        emit needSaveChanged(m_needSave);
    }
}

bool AbstractItemEditorModel::needSave()
{
    return m_needSave;
}

}