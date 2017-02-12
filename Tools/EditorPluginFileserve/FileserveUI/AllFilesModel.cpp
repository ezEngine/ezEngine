#include <PCH.h>
#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>
#include <QTimer>
#include <QWidget>

ezQtFileserveAllFilesModel::ezQtFileserveAllFilesModel(QWidget* parent)
  : QAbstractListModel(parent)
{
}

int ezQtFileserveAllFilesModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
  return m_IndexedFiles.GetCount() - m_uiAddedItems;
}

int ezQtFileserveAllFilesModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant ezQtFileserveAllFilesModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole)
    {
      return QString::number(m_IndexedFiles[index.row()].Value());
    }
  }

  if (index.column() == 1)
  {
    if (role == Qt::DisplayRole)
    {
      return m_IndexedFiles[index.row()].Key().GetData();
    }
  }

  return QVariant();
}


void ezQtFileserveAllFilesModel::AddAccessedFile(const char* szFile)
{
  bool bExisted = false;
  auto& it = m_AllFiles.FindOrAdd(szFile, &bExisted);

  // we count the accesses, but for performance reasons the TableView is not updated when the counter changes
  it.Value()++;

  if (!bExisted)
  {
    m_IndexedFiles.PushBack(it);
    ++m_uiAddedItems;

    if (!m_bTimerRunning)
    {
      m_bTimerRunning = true;
      QTimer::singleShot(500, this, &ezQtFileserveAllFilesModel::UpdateViewSlot);
    }
  }
}

void ezQtFileserveAllFilesModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_IndexedFiles.GetCount() - m_uiAddedItems, m_IndexedFiles.GetCount() - 1);
  insertRows(m_IndexedFiles.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void ezQtFileserveAllFilesModel::Clear()
{
  m_AllFiles.Clear();
  m_IndexedFiles.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void ezQtFileserveAllFilesModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}

QVariant ezQtFileserveAllFilesModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    if (section == 0)
    {
      return "Accesses";
    }

    if (section == 1)
    {
      return "File";
    }
  }

  return QVariant();
}


