#include <PCH.h>
#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>
#include <QTimer>
#include <QWidget>

ezQtFileserveActivityModel::ezQtFileserveActivityModel(QWidget* parent)
  : QAbstractListModel(parent)
{
}

int ezQtFileserveActivityModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
  return m_Items.GetCount() - m_uiAddedItems;
}

int ezQtFileserveActivityModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant ezQtFileserveActivityModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  const auto& item = m_Items[index.row()];

  if (role == Qt::ToolTipRole)
  {
    if (item.m_Type == ezFileserveActivityType::ReadFile)
    {
      return QString("[TIME] == File was not transferred because the timestamps match on server and client.\n"
                     "[HASH] == File was not transferred because the file hashes matched on server and client.\n"
                     "[N/A] == File does not exist on the server (in the requested data directory).");
    }
  }

  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole)
    {
      switch (item.m_Type)
      {
      case ezFileserveActivityType::StartServer:
        return "Server Started";
      case ezFileserveActivityType::StopServer:
        return "Server Stopped";
      case ezFileserveActivityType::ClientConnect:
        return "Client Connected";
      case ezFileserveActivityType::ClientReconnected:
        return "Client Re-connected";
      case ezFileserveActivityType::ClientDisconnect:
        return "Client Disconnect";
      case ezFileserveActivityType::Mount:
        return "Mount";
      case ezFileserveActivityType::MountFailed:
        return "Failed Mount";
      case ezFileserveActivityType::Unmount:
        return "Unmount";
      case ezFileserveActivityType::ReadFile:
        return "Read";
      case ezFileserveActivityType::WriteFile:
        return "Write";
      case ezFileserveActivityType::DeleteFile:
        return "Delete";

      default:
        return QVariant();
      }
    }

    if (role == Qt::TextColorRole)
    {
      switch (item.m_Type)
      {
      case ezFileserveActivityType::StartServer:
        return QColor::fromRgb(0, 200, 0);
      case ezFileserveActivityType::StopServer:
        return QColor::fromRgb(200, 200, 0);

      case ezFileserveActivityType::ClientConnect:
      case ezFileserveActivityType::ClientReconnected:
        return QColor::fromRgb(50, 200, 0);
      case ezFileserveActivityType::ClientDisconnect:
        return QColor::fromRgb(250, 100, 0);

      case ezFileserveActivityType::Mount:
        return QColor::fromRgb(0, 0, 200);
      case ezFileserveActivityType::MountFailed:
        return QColor::fromRgb(255, 0, 0);
      case ezFileserveActivityType::Unmount:
        return QColor::fromRgb(150, 0, 200);

      case ezFileserveActivityType::ReadFile:
        return QColor::fromRgb(100, 100, 100);
      case ezFileserveActivityType::WriteFile:
        return QColor::fromRgb(255, 150, 0);
      case ezFileserveActivityType::DeleteFile:
        return QColor::fromRgb(200, 50, 50);

      default:
        return QVariant();
      }
    }
  }

  if (index.column() == 1)
  {
    if (role == Qt::DisplayRole)
    {
      return item.m_Text;
    }
  }

  return QVariant();
}


QVariant ezQtFileserveActivityModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    if (section == 0)
    {
      return "Type";
    }

    if (section == 1)
    {
      return "Action";
    }
  }

  return QVariant();
}

ezQtFileserveActivityItem& ezQtFileserveActivityModel::AppendItem()
{
  if (!m_bTimerRunning)
  {
    m_bTimerRunning = true;

    QTimer::singleShot(250, this, &ezQtFileserveActivityModel::UpdateViewSlot);
  }

  m_uiAddedItems++;
  return m_Items.ExpandAndGetRef();
}

void ezQtFileserveActivityModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_Items.GetCount() - m_uiAddedItems, m_Items.GetCount() - 1);
  insertRows(m_Items.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void ezQtFileserveActivityModel::Clear()
{
  m_Items.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void ezQtFileserveActivityModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}

