#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <EditorFramework/EditorGUI.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <QStringList>
#include <QMimeData>
#include <QMessageBox>

ezRawDocumentTreeModel::ezRawDocumentTreeModel(const ezDocumentObjectTree* pTree) :
  QAbstractItemModel(nullptr)
{
  m_pDocumentTree = pTree;

  m_pDocumentTree->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezRawDocumentTreeModel::TreeEventHandler, this));
  m_pDocumentTree->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezRawDocumentTreeModel::TreePropertyEventHandler, this));

  
}

ezRawDocumentTreeModel::~ezRawDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezRawDocumentTreeModel::TreeEventHandler, this));
  m_pDocumentTree->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezRawDocumentTreeModel::TreePropertyEventHandler, this));
}

void ezRawDocumentTreeModel::TreePropertyEventHandler(const ezDocumentObjectTreePropertyEvent& e)
{
  if (!e.m_bEditorProperty)
    return;

  if (e.m_sPropertyPath != "Name")
    return;

  auto index = ComputeModelIndex(e.m_pObject);
  QVector<int> v;
  v.push_back(Qt::DisplayRole);
  dataChanged(index, index, v);
}

void ezRawDocumentTreeModel::TreeEventHandler(const ezDocumentObjectTreeStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectAdded:
    {
      ezInt32 iIndex = (ezInt32)e.m_uiNewChildIndex;
      if (e.m_pNewParent == m_pDocumentTree->GetRootObject())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectRemoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
  case ezDocumentObjectTreeStructureEvent::Type::BeforeObjectMoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), e.m_uiNewChildIndex);
    }
    break;
  case ezDocumentObjectTreeStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
  }
}

QModelIndex ezRawDocumentTreeModel::index(int row, int column, const QModelIndex& parent) const
{
  if (!parent.isValid())
  {
    if (m_pDocumentTree->GetRootObject()->GetChildren().IsEmpty())
      return QModelIndex();

    ezDocumentObjectBase* pObject = m_pDocumentTree->GetRootObject()->GetChildren()[row];
    return createIndex(row, column, pObject);
  }

  const ezDocumentObjectBase* pParent = (const ezDocumentObjectBase*) parent.internalPointer();

  return createIndex(row, column, pParent->GetChildren()[row]);
}

ezInt32 ezRawDocumentTreeModel::ComputeIndex(const ezDocumentObjectBase* pObject) const
{
  const ezDocumentObjectBase* pParent = pObject->GetParent();

  ezInt32 iIndex = pParent->GetChildren().IndexOf((ezDocumentObjectBase*) pObject);

  return iIndex;
}

QModelIndex ezRawDocumentTreeModel::ComputeModelIndex(const ezDocumentObjectBase* pObject) const
{
  if (pObject == m_pDocumentTree->GetRootObject())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}

QModelIndex ezRawDocumentTreeModel::ComputeParent(const ezDocumentObjectBase* pObject) const
{
  const ezDocumentObjectBase* pParent = pObject->GetParent();

  if (pParent == m_pDocumentTree->GetRootObject())
    return QModelIndex();

  ezInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, (ezDocumentObjectBase*) pParent);
}

QModelIndex ezRawDocumentTreeModel::parent(const QModelIndex& child) const
{
  const ezDocumentObjectBase* pObject = (const ezDocumentObjectBase*) child.internalPointer();

  return ComputeParent(pObject);
}

int ezRawDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;

  if (!parent.isValid())
  {
    iCount = m_pDocumentTree->GetRootObject()->GetChildren().GetCount();
  }
  else
  {
    const ezDocumentObjectBase* pObject = (const ezDocumentObjectBase*) parent.internalPointer();
  
    iCount = pObject->GetChildren().GetCount();
  }

  return iCount;
}

int ezRawDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant ezRawDocumentTreeModel::data(const QModelIndex& index, int role) const
{
  //if (index.isValid())
  {
    const ezDocumentObjectBase* pObject = (const ezDocumentObjectBase*) index.internalPointer();

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetEditorTypeAccessor().GetValue(ezToolsReflectionUtils::CreatePropertyPath("Name")).ConvertTo<ezString>().GetData());
      }
      break;
    }
  }

  return QVariant();
}

Qt::DropActions ezRawDocumentTreeModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::ItemFlags ezRawDocumentTreeModel::flags(const QModelIndex &index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  if (index.column() == 0)
  {
    return (/*Qt::ItemIsUserCheckable | */ Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

bool ezRawDocumentTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  if (column > 0)
    return false;

  if (data->hasFormat("application/ezEditor.ObjectSelection"))
  {
    ezDocumentObjectBase* pNewParent = (ezDocumentObjectBase*) parent.internalPointer();

    ezHybridArray<ezDocumentObjectBase*, 32> Dragged;

    QByteArray encodedData = data->data("application/ezEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);

    int iIndices = 0;
    stream >> iIndices;

    for (int i = 0; i < iIndices; ++i)
    {
      void* p = nullptr;

      uint len = sizeof(void*);
      stream.readRawData((char*) &p, len);

      ezDocumentObjectBase* pDocObject = (ezDocumentObjectBase*) p;

      Dragged.PushBack(pDocObject);

      if (action != Qt::DropAction::MoveAction)
      {
        bool bCanMove = true;
        const ezDocumentObjectBase* pCurParent = pNewParent;

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
          ezEditorGUI::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pDoc = m_pDocumentTree->GetDocument();
    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction();

    for (ezUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      ezMoveObjectCommand cmd;
      cmd.m_Object = Dragged[i]->GetGuid();
      cmd.m_iNewChildIndex = row;

      if (pNewParent)
        cmd.m_NewParent = pNewParent->GetGuid();

      pHistory->AddCommand(cmd);
    }

    pHistory->EndTransaction(false);

    return true;
  }

  if (data->hasFormat("application/ezEditor.ObjectCreator"))
  {
    QByteArray encodedData = data->data("application/ezEditor.ObjectCreator");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QString typeName;
    stream >> typeName;

    ezAddObjectCommand cmd;
    cmd.SetType(typeName.toUtf8().data());
    cmd.m_iChildIndex = row;

    if (parent.isValid())
      cmd.m_Parent = ((ezDocumentObjectBase*) parent.internalPointer())->GetGuid();

    auto history = m_pDocumentTree->GetDocument()->GetCommandHistory();

    history->StartTransaction();

    ezStatus ret = history->AddCommand(cmd);

    history->EndTransaction(ret.m_Result.Failed());
  }

  return false;
}

QStringList ezRawDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  types << "application/ezEditor.ObjectSelection";
  types << "application/ezEditor.ObjectCreator";
  return types;
}

QMimeData* ezRawDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  int iCount = 0;

  foreach(QModelIndex index, indexes)
  {
    if (index.isValid())
      ++iCount;
  }

  stream << iCount;

  foreach(QModelIndex index, indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      stream.writeRawData((const char*) &pObject, sizeof(void*));
    }
  }

  mimeData->setData("application/ezEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool ezRawDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
  if (role == Qt::EditRole)
  {
    const ezDocumentObjectBase* pObject = (const ezDocumentObjectBase*) index.internalPointer();

    auto pHistory = m_pDocumentTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction();

    ezSetObjectPropertyCommand cmd;
    cmd.m_bEditorProperty = true;
    cmd.m_NewValue = value.toString().toUtf8().data();
    cmd.m_Object = pObject->GetGuid();
    cmd.SetPropertyPath("Name");

    pHistory->AddCommand(cmd);

    pHistory->EndTransaction(false);

    QVector<int> roles;
    roles.push_back(Qt::DisplayRole);
    roles.push_back(Qt::EditRole);

    emit dataChanged(index, index, roles);
  }

  return false;
}

