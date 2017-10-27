#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

ezQtPropertyAnimModel::ezQtPropertyAnimModel(ezPropertyAnimAssetDocument* pDocument, QObject* pParent)
  : QAbstractItemModel(pParent)
  , m_pAssetDoc(pDocument)
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentObjectEventHandler, this));
  m_pAssetDoc->GetObjectManager() ->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentStructureEventHandler, this));

  BuildMapping();
}

ezQtPropertyAnimModel::~ezQtPropertyAnimModel()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentObjectEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentStructureEventHandler, this));
}

QVariant ezQtPropertyAnimModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  TreeEntry* pItem = static_cast<TreeEntry*>(index.internalPointer());
  EZ_ASSERT_DEBUG(pItem != nullptr, "Invalid model index");

  switch (role)
  {
  case Qt::DisplayRole:
    return QString(pItem->m_sDisplay.GetData());

  case UserRoles::TrackPtr:
    return qVariantFromValue((void*)pItem->m_pTrack);
  }

  return QVariant();
}

Qt::ItemFlags ezQtPropertyAnimModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex ezQtPropertyAnimModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (column != 0)
    return QModelIndex();

  TreeEntry* pParentItem = static_cast<TreeEntry*>(parent.internalPointer());
  if (pParentItem != nullptr)
  {
    return createIndex(row, column, (void*)pParentItem->m_Children[row]);
  }
  else
  {
    if (row >= (int)m_TopLevelEntries.GetCount())
      return QModelIndex();

    return createIndex(row, column, (void*)m_TopLevelEntries[row]);
  }
}

QModelIndex ezQtPropertyAnimModel::parent(const QModelIndex& index) const
{
  if (!index.isValid() || index.column() != 0)
    return QModelIndex();

  TreeEntry* pItem = static_cast<TreeEntry*>(index.internalPointer());

  if (pItem->m_pParent == nullptr)
    return QModelIndex();

  return createIndex(pItem->m_pParent->m_uiOwnRowIndex, index.column(), (void*)pItem->m_pParent);
}

int ezQtPropertyAnimModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (!parent.isValid())
    return m_TopLevelEntries.GetCount();

  TreeEntry* pItem = static_cast<TreeEntry*>(parent.internalPointer());
  return pItem->m_Children.GetCount();
}

int ezQtPropertyAnimModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 1;
}

void ezQtPropertyAnimModel::DocumentObjectEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  beginResetModel();

  //if (e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertyInserted ||
  //  e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertyMoved ||
  //  e.m_EventType == ezDocumentObjectPropertyEvent::Type::PropertyRemoved)
  //{
    BuildMapping();
  //}

  endResetModel();
}


void ezQtPropertyAnimModel::DocumentStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    {
      beginResetModel();
      BuildMapping();
      endResetModel();
    }
    break;
  }
}

void ezQtPropertyAnimModel::BuildMapping()
{
  m_TopLevelEntries.Clear();

  const ezPropertyAnimationTrackGroup& group = *m_pAssetDoc->GetProperties();

  for (ezUInt32 tIdx = 0; tIdx < group.m_Tracks.GetCount(); ++tIdx)
  {
    ezPropertyAnimationTrack* pTrack = group.m_Tracks[tIdx];
    BuildMapping(pTrack, m_TopLevelEntries, nullptr, pTrack->m_sPropertyName);
  }
}

void ezQtPropertyAnimModel::BuildMapping(ezPropertyAnimationTrack* pTrack, ezDynamicArray<TreeEntry*>& treeItems, TreeEntry* pParentEntry, const char* szPath)
{
  const char* szSubPath = ezStringUtils::FindSubString(szPath, "/");

  ezStringBuilder name;

  if (szSubPath != nullptr)
    name.SetSubString_FromTo(szPath, szSubPath);
  else
    name = szPath;

  TreeEntry* pThisEntry = nullptr;

  for (ezUInt32 i = 0; i < treeItems.GetCount(); ++i)
  {
    if (treeItems[i]->m_sDisplay.IsEqual_NoCase(name))
    {
      pThisEntry = treeItems[i];
      break;
    }
  }

  if (pThisEntry == nullptr)
  {
    pThisEntry = &m_AllEntries.ExpandAndGetRef();
    treeItems.PushBack(pThisEntry);

    pThisEntry->m_pParent = pParentEntry;
    pThisEntry->m_sDisplay = name;
    pThisEntry->m_uiOwnRowIndex = treeItems.GetCount() - 1;
  }

  if (szSubPath != nullptr)
  {
    szSubPath += 1;
    BuildMapping(pTrack, pThisEntry->m_Children, pThisEntry, szSubPath);
  }
  else
  {
    pThisEntry->m_pTrack = pTrack;
  }
}

