#include <PCH.h>
#include <InputTest/InputListModel.moc.h>
#include <Core/Input/InputManager.h>

InputListModel* g_pFileListModel = NULL;

InputListModel::InputListModel(QObject *parent) : QAbstractTableModel(parent) 
{
  
}

int InputListModel::rowCount(const QModelIndex& parent) const 
{
  ezDynamicArray<const char*> Slots;
  ezInputManager::RetrieveAllKnownInputSlots(Slots);
  return Slots.GetCount();
}

int InputListModel::columnCount(const QModelIndex& parent) const 
{
  return 5;
}

QVariant InputListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Vertical)
    return QVariant();

  if (role == Qt::DisplayRole)
  {
    if (section == 0)
      return QString("Engine Name");
    if (section == 1)
      return QString("Display Name");
    if (section == 2)
      return QString("State");
    if (section == 3)
      return QString("Value");
    if (section == 4)
      return QString("Deadzone");

    return QVariant();
  }

  return QVariant();
}

QVariant InputListModel::data(const QModelIndex& index, int role) const 
{
  //if (!index.isValid() || index.row() < 0 || index.row() >= g_State.GetWallpaperSets()[m_iSelectedSet].m_Items.size())
    //return QVariant();

  if (role == Qt::ToolTipRole)
  {
    return QVariant();
  }

  if (role == Qt::DisplayRole)
  {
    ezDynamicArray<const char*> Slots;
    ezInputManager::RetrieveAllKnownInputSlots(Slots);

    if (Slots.IsEmpty())
      return QVariant();


    if (index.column() == 0)
      return QVariant(QString::fromUtf8(Slots[index.row()]));

    if (index.column() == 1)
      return QVariant(QString::fromUtf8(ezInputManager::GetInputSlotDisplayName(Slots[index.row()])));

    if (index.column() == 2)
    {
      switch (ezInputManager::GetInputSlotState(Slots[index.row()]))
      {
      case ezKeyState::Down:
        return QString("Down");
      case ezKeyState::Up:
        return QString("Up");
      case ezKeyState::Pressed:
        return QString("Pressed");
      case ezKeyState::Released:
        return QString("Released");
      }
    }

    if (index.column() == 3)
    {
      float val = -1.0f;
      ezInputManager::GetInputSlotState(Slots[index.row()], &val);
      return val;
    }

    if (index.column() == 4)
      return QVariant(ezInputManager::GetInputSlotDeadZone(Slots[index.row()]));
  }

  if (role == Qt::BackgroundColorRole)
  {
    ezDynamicArray<const char*> Slots;
    ezInputManager::RetrieveAllKnownInputSlots(Slots);

    if (Slots.IsEmpty())
      return QVariant();

    //switch (g_InputHandler.GetStatic().GetKeyData((ezInputKey::Enum) index.row()).GetState())
    switch (ezInputManager::GetInputSlotState(Slots[index.row()]))
    {
    case ezKeyState::Down:
      return QVariant(QColor(Qt::green));
    case ezKeyState::Pressed:
      return QVariant(QColor(Qt::red));
    case ezKeyState::Released:
      return QVariant(QColor(Qt::yellow));
    }

    return QVariant();
  }

  if (role == Qt::DecorationRole)
  {
    return QVariant();
  }

  return QVariant();
}


Qt::ItemFlags InputListModel::flags(const QModelIndex& index) const
{
  Qt::ItemFlags DefaultFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  return DefaultFlags;
}

