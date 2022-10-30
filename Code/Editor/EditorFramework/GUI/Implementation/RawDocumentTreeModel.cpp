#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

ezQtDocumentTreeModelAdapter::ezQtDocumentTreeModelAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* sChildProperty)
  : m_pTree(pTree)
  , m_pType(pType)
  , m_sChildProperty(sChildProperty)
{
  if (!m_sChildProperty.IsEmpty())
  {
    auto pProp = pType->FindPropertyByName(m_sChildProperty);
    EZ_ASSERT_DEV(pProp != nullptr && (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set),
      "The visualized object property tree must either be a set or array!");
    EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer) || pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner),
      "The visualized object must have ownership of the property objects!");
  }
}

const ezRTTI* ezQtDocumentTreeModelAdapter::GetType() const
{
  return m_pType;
}


const ezString& ezQtDocumentTreeModelAdapter::GetChildProperty() const
{
  return m_sChildProperty;
}

bool ezQtDocumentTreeModelAdapter::setData(const ezDocumentObject* pObject, int row, int column, const QVariant& value, int role) const
{
  return false;
}

Qt::ItemFlags ezQtDocumentTreeModelAdapter::flags(const ezDocumentObject* pObject, int row, int column) const
{
  if (column == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}


ezQtDummyAdapter::ezQtDummyAdapter(const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty)
  : ezQtDocumentTreeModelAdapter(pTree, pType, m_sChildProperty)
{
}

QVariant ezQtDummyAdapter::data(const ezDocumentObject* pObject, int row, int column, int role) const
{
  if (column == 0)
  {
    switch (role)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetType()->GetTypeName());
      }
      break;
    }
  }
  return QVariant();
}

ezQtNamedAdapter::ezQtNamedAdapter(
  const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty, const char* szNameProperty)
  : ezQtDocumentTreeModelAdapter(pTree, pType, m_sChildProperty)
  , m_sNameProperty(szNameProperty)
{
  auto pProp = pType->FindPropertyByName(m_sNameProperty);
  EZ_ASSERT_DEV(
    pProp != nullptr && pProp->GetCategory() == ezPropertyCategory::Member && pProp->GetSpecificType()->GetVariantType() == ezVariantType::String,
    "THe name property must be a string member property.");

  m_pTree->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtNamedAdapter::TreePropertyEventHandler, this));
}

ezQtNamedAdapter::~ezQtNamedAdapter()
{
  m_pTree->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtNamedAdapter::TreePropertyEventHandler, this));
}

QVariant ezQtNamedAdapter::data(const ezDocumentObject* pObject, int row, int column, int role) const
{
  if (column == 0)
  {
    switch (role)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetValue(m_sNameProperty).ConvertTo<ezString>().GetData());
      }
      break;
    }
  }
  return QVariant();
}

void ezQtNamedAdapter::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == m_sNameProperty)
  {
    QVector<int> v;
    v.push_back(Qt::DisplayRole);
    v.push_back(Qt::EditRole);
    Q_EMIT dataChanged(e.m_pObject, v);
  }
}

ezQtNameableAdapter::ezQtNameableAdapter(
  const ezDocumentObjectManager* pTree, const ezRTTI* pType, const char* m_sChildProperty, const char* szNameProperty)
  : ezQtNamedAdapter(pTree, pType, m_sChildProperty, szNameProperty)
{
}

ezQtNameableAdapter::~ezQtNameableAdapter() {}

bool ezQtNameableAdapter::setData(const ezDocumentObject* pObject, int row, int column, const QVariant& value, int role) const
{
  if (column == 0 && role == Qt::EditRole)
  {
    auto pHistory = m_pTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction(ezFmt("Rename to '{0}'", value.toString().toUtf8().data()));

    ezSetObjectPropertyCommand cmd;
    cmd.m_NewValue = value.toString().toUtf8().data();
    cmd.m_Object = pObject->GetGuid();
    cmd.m_sProperty = m_sNameProperty;

    pHistory->AddCommand(cmd);

    pHistory->FinishTransaction();

    return true;
  }
  return false;
}

Qt::ItemFlags ezQtNameableAdapter::flags(const ezDocumentObject* pObject, int row, int column) const
{
  if (column == 0)
  {
    return (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////

ezQtDocumentTreeModel::ezQtDocumentTreeModel(const ezDocumentObjectManager* pTree, const ezUuid& root)
  : QAbstractItemModel(nullptr)
  , m_pDocumentTree(pTree)
  , m_Root(root)
{
  m_pDocumentTree->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreeEventHandler, this));
}

ezQtDocumentTreeModel::~ezQtDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtDocumentTreeModel::TreeEventHandler, this));
}

void ezQtDocumentTreeModel::AddAdapter(ezQtDocumentTreeModelAdapter* adapter)
{
  EZ_ASSERT_DEV(!m_Adapters.Contains(adapter->GetType()), "An adapter for the given type was already registered.");

  adapter->setParent(this);
  connect(adapter, &ezQtDocumentTreeModelAdapter::dataChanged, this, [this](const ezDocumentObject* pObject, QVector<int> roles) {
    if (!pObject)
      return;
    auto index = ComputeModelIndex(pObject);
    if (!index.isValid())
      return;
    dataChanged(index, index, roles);
  });
  m_Adapters.Insert(adapter->GetType(), adapter);
  beginResetModel();
  endResetModel();
}

const ezQtDocumentTreeModelAdapter* ezQtDocumentTreeModel::GetAdapter(const ezRTTI* pType) const
{
  while (pType != nullptr)
  {
    if (const ezQtDocumentTreeModelAdapter* const* adapter = m_Adapters.GetValue(pType))
    {
      return *adapter;
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}

void ezQtDocumentTreeModel::TreeEventHandler(const ezDocumentObjectStructureEvent& e)
{
  const ezDocumentObject* pParent = nullptr;
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::BeforeReset:
      beginResetModel();
      return;
    case ezDocumentObjectStructureEvent::Type::AfterReset:
      endResetModel();
      return;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      pParent = e.m_pPreviousParent;
      break;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      pParent = e.m_pNewParent;
      break;
  }
  EZ_ASSERT_DEV(pParent != nullptr, "Each structure event should have a parent set.");
  if (!IsUnderRoot(pParent))
    return;
  auto pType = pParent->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return;

  if (pAdapter->GetChildProperty() != e.m_sParentProperty)
    return;

  // TODO: BLA root object could have other objects instead of m_pBaseClass, in which case indices are broken on root.

  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      ezInt32 iIndex = (ezInt32)e.m_NewPropertyIndex.ConvertTo<ezInt32>();
      if (e.m_pNewParent == GetRoot())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      ezInt32 iNewIndex = (ezInt32)e.m_NewPropertyIndex.ConvertTo<ezInt32>();
      ezInt32 iIndex = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
    default:
      break;
  }
}

QModelIndex ezQtDocumentTreeModel::index(int row, int column, const QModelIndex& parent) const
{
  const ezDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const ezDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();
  if (row >= pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty()))
    return QModelIndex();

  ezVariant value = pObject->GetTypeAccessor().GetValue(pAdapter->GetChildProperty(), row);
  EZ_ASSERT_DEV(value.IsValid() && value.IsA<ezUuid>(), "Tree corruption!");
  const ezDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<ezUuid>());
  return createIndex(row, column, const_cast<ezDocumentObject*>(pChild));
}

ezInt32 ezQtDocumentTreeModel::ComputeIndex(const ezDocumentObject* pObject) const
{
  ezInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<ezInt32>();
  return iIndex;
}

const ezDocumentObject* ezQtDocumentTreeModel::GetRoot() const
{
  if (m_Root.IsValid())
  {
    return m_pDocumentTree->GetObject(m_Root);
  }
  return m_pDocumentTree->GetRootObject();
}

bool ezQtDocumentTreeModel::IsUnderRoot(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pRoot = GetRoot();
  while (pObject)
  {
    if (pRoot == pObject)
      return true;

    pObject = pObject->GetParent();
  }
  return false;
}

QModelIndex ezQtDocumentTreeModel::ComputeModelIndex(const ezDocumentObject* pObject) const
{
  // Filter out objects that are not under the child property of the
  // parents adapter.
  if (pObject == GetRoot())
    return QModelIndex();

  auto pType = pObject->GetParent()->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();

  if (pAdapter->GetChildProperty() != pObject->GetParentProperty())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}


void ezQtDocumentTreeModel::SetAllowDragDrop(bool bAllow)
{
  m_bAllowDragDrop = bAllow;
}

QModelIndex ezQtDocumentTreeModel::ComputeParent(const ezDocumentObject* pObject) const
{
  const ezDocumentObject* pParent = pObject->GetParent();

  if (pParent == GetRoot())
    return QModelIndex();

  ezInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, const_cast<ezDocumentObject*>(pParent));
}

QModelIndex ezQtDocumentTreeModel::parent(const QModelIndex& child) const
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)child.internalPointer();

  return ComputeParent(pObject);
}

int ezQtDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;
  const ezDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const ezDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    if (!pAdapter->GetChildProperty().IsEmpty())
    {
      iCount = pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty());
    }
  }

  return iCount;
}

int ezQtDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant ezQtDocumentTreeModel::data(const QModelIndex& index, int role) const
{
  // if (index.isValid())
  {
    const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();
    auto pType = pObject->GetTypeAccessor().GetType();
    if (auto pAdapter = GetAdapter(pType))
    {
      return pAdapter->data(pObject, index.row(), index.column(), role);
    }
  }

  return QVariant();
}

Qt::DropActions ezQtDocumentTreeModel::supportedDropActions() const
{
  if (m_bAllowDragDrop)
    return Qt::MoveAction | Qt::CopyAction;

  return Qt::IgnoreAction;
}

Qt::ItemFlags ezQtDocumentTreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->flags(pObject, index.row(), index.column());
  }

  return Qt::ItemFlag::NoItemFlags;
}


bool ezQtDocumentTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
  const ezDocumentObject* pNewParent = (const ezDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  ezDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = row;
  info.m_pMimeData = data;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());

  if (ezDragDropHandler::CanDropOnly(&info))
    return true;

  {
    // Test 'CanMove' of the target object manager.
    QByteArray encodedData = data->data("application/ezEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    ezHybridArray<ezDocumentObject*, 32> Dragged;
    stream >> Dragged;

    auto pType = pNewParent->GetTypeAccessor().GetType();
    auto pAdapter = GetAdapter(pType);
    const ezString& sProperty = pAdapter->GetChildProperty();
    for (const ezDocumentObject* pItem : Dragged)
    {
      // If the item's and the target tree's document don't match we can't operate via this code.
      if (pItem->GetDocumentObjectManager()->GetDocument() != m_pDocumentTree->GetDocument())
        return false;
      if (m_pDocumentTree->CanMove(pItem, pNewParent, sProperty, info.m_iTargetObjectInsertChildIndex).Failed())
        return false;
    }
    return QAbstractItemModel::canDropMimeData(data, action, row, column, parent);
  }
  return false;
}

bool ezQtDocumentTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  if (!m_bAllowDragDrop)
    return false;

  if (column > 0)
    return false;

  const ezDocumentObject* pNewParent = (const ezDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  ezDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = row;
  info.m_pMimeData = data;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());
  if (ezDragDropHandler::DropOnly(&info))
    return true;

  return ezQtDocumentTreeModel::MoveObjects(info);
}


bool ezQtDocumentTreeModel::MoveObjects(const ezDragDropInfo& info)
{
  if (info.m_pMimeData->hasFormat("application/ezEditor.ObjectSelection"))
  {
    auto pDoc = ezDocumentManager::GetDocumentByGuid(info.m_TargetDocument);
    const ezDocumentObject* pTarget = pDoc->GetObjectManager()->GetObject(info.m_TargetObject);

    QByteArray encodedData = info.m_pMimeData->data("application/ezEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    ezHybridArray<ezDocumentObject*, 32> Dragged;
    stream >> Dragged;

    for (const ezDocumentObject* pDocObject : Dragged)
    {
      //if (action != Qt::DropAction::MoveAction)
      {
        bool bCanMove = true;
        const ezDocumentObject* pCurParent = pTarget;

        while (pCurParent)
        {
          if (pCurParent == pDocObject)
          {
            bCanMove = false;
            break;
          }

          pCurParent = pCurParent->GetParent();
        }

        if (!bCanMove)
        {
          ezQtUiServices::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Reparent Object");

    ezStatus res(EZ_SUCCESS);
    for (ezUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      ezMoveObjectCommand cmd;
      cmd.m_Object = Dragged[i]->GetGuid();
      cmd.m_Index = info.m_iTargetObjectInsertChildIndex;
      cmd.m_sParentProperty = info.m_pAdapter->GetChildProperty();
      cmd.m_NewParent = pTarget->GetGuid();

      res = pHistory->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      pHistory->CancelTransaction();
    else
      pHistory->FinishTransaction();

    ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node move failed.");
    return true;
  }

  return false;
}

QStringList ezQtDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  if (m_bAllowDragDrop)
  {
    types << "application/ezEditor.ObjectSelection";
  }

  return types;
}

QMimeData* ezQtDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  if (!m_bAllowDragDrop)
    return nullptr;

  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  int iCount = 0;

  foreach (QModelIndex index, indexes)
  {
    if (index.isValid())
      ++iCount;
  }

  stream << iCount;

  foreach (QModelIndex index, indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      stream.writeRawData((const char*)&pObject, sizeof(void*));
    }
  }

  mimeData->setData("application/ezEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool ezQtDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  const ezDocumentObject* pObject = (const ezDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->setData(pObject, index.row(), index.column(), value, role);
  }

  return false;
}
