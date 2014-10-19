#include <PCH.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <QStringList>
#include <QMimeData>

ezRawDocumentTreeModel::ezRawDocumentTreeModel(const ezDocumentObjectTree* pTree) :
  QAbstractItemModel(nullptr)
{
  m_pDocumentTree = pTree;

  m_pDocumentTree->m_Events.AddEventHandler(ezDelegate<void (const ezDocumentObjectTree::Event&)>(&ezRawDocumentTreeModel::TreeEventHandler, this));
}

ezRawDocumentTreeModel::~ezRawDocumentTreeModel()
{
  m_pDocumentTree->m_Events.RemoveEventHandler(ezDelegate<void (const ezDocumentObjectTree::Event&)>(&ezRawDocumentTreeModel::TreeEventHandler, this));
}

void ezRawDocumentTreeModel::TreeEventHandler(const ezDocumentObjectTree::Event& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectTree::Event::Type::BeforeObjectAdded:
    {
      ezInt32 iIndex = e.m_pNewParent->GetChildren().GetCount();
      if (e.m_pNewParent == m_pDocumentTree->GetRootObject())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectTree::Event::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
  case ezDocumentObjectTree::Event::Type::BeforeObjectRemoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
  case ezDocumentObjectTree::Event::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
  case ezDocumentObjectTree::Event::Type::BeforeObjectMoved:
    {
      ezInt32 iIndex = ComputeIndex(e.m_pObject);
      ezInt32 iNewIndex = e.m_pNewParent->GetChildren().GetCount();
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
  case ezDocumentObjectTree::Event::Type::AfterObjectMoved:
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
    return (/*Qt::ItemIsUserCheckable | Qt::ItemIsEditable |*/ Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

bool ezRawDocumentTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
  if (column > 0)
    return false;

  if (data->hasFormat("application/ezEditor.ObjectSelection"))
  {
    if (action != Qt::DropAction::MoveAction)
      return false;

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

      ((ezDocumentObjectTree*) m_pDocumentTree)->MoveObject(pDocObject, (ezDocumentObjectBase*) parent.internalPointer());
    }

    return true;
  }

  if (data->hasFormat("application/ezEditor.ObjectCreator"))
  {
    QByteArray encodedData = data->data("application/ezEditor.ObjectCreator");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QString typeName;
    stream >> typeName;
    ezDocumentObjectBase* pObject = ((ezDocumentObjectTree*)m_pDocumentTree)->GetObjectManager()->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(typeName.toUtf8().data()));
    if (pObject != nullptr)
    {
      if (parent.isValid())
      {
        ((ezDocumentObjectTree*) m_pDocumentTree)->AddObject(pObject, (ezDocumentObjectBase*) parent.internalPointer());
      }
      else
      {
        ((ezDocumentObjectTree*) m_pDocumentTree)->AddObject(pObject, nullptr);
      }
      
    }
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