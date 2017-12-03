#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QTimer>

ezQtPropertyAnimModel::ezQtPropertyAnimModel(ezPropertyAnimAssetDocument* pDocument, QObject* pParent)
  : QAbstractItemModel(pParent)
  , m_pAssetDoc(pDocument)
{
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentStructureEventHandler, this));

  TriggerBuildMapping();
}

ezQtPropertyAnimModel::~ezQtPropertyAnimModel()
{
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezQtPropertyAnimModel::DocumentStructureEventHandler, this));
}

QVariant ezQtPropertyAnimModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  ezQtPropertyAnimModelTreeEntry* pItem = static_cast<ezQtPropertyAnimModelTreeEntry*>(index.internalPointer());
  EZ_ASSERT_DEBUG(pItem != nullptr, "Invalid model index");

  switch (role)
  {
  case Qt::DisplayRole:
    return QString(pItem->m_sDisplay.GetData());

  case Qt::DecorationRole:
    return pItem->m_Icon;

  case UserRoles::TrackPtr:
    return qVariantFromValue((void*)pItem->m_pTrack);

  case UserRoles::TreeItem:
    return qVariantFromValue((void*)pItem);

  case UserRoles::TrackIdx:
    return pItem->m_iTrackIdx;

  case UserRoles::Path:
    return QString(pItem->m_sPathToItem.GetData());
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

  ezQtPropertyAnimModelTreeEntry* pParentItem = static_cast<ezQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  if (pParentItem != nullptr)
  {
    return createIndex(row, column, (void*)&m_AllEntries[m_iInUse][pParentItem->m_Children[row]]);
  }
  else
  {
    if (row >= (int)m_TopLevelEntries[m_iInUse].GetCount())
      return QModelIndex();

    return createIndex(row, column, (void*)&m_AllEntries[m_iInUse][m_TopLevelEntries[m_iInUse][row]]);
  }
}

QModelIndex ezQtPropertyAnimModel::parent(const QModelIndex& index) const
{
  if (!index.isValid() || index.column() != 0)
    return QModelIndex();

  ezQtPropertyAnimModelTreeEntry* pItem = static_cast<ezQtPropertyAnimModelTreeEntry*>(index.internalPointer());

  if (pItem->m_iParent < 0)
    return QModelIndex();

  return createIndex(m_AllEntries[m_iInUse][pItem->m_iParent].m_uiOwnRowIndex, index.column(), (void*)&m_AllEntries[m_iInUse][pItem->m_iParent]);
}

int ezQtPropertyAnimModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (!parent.isValid())
    return m_TopLevelEntries[m_iInUse].GetCount();

  ezQtPropertyAnimModelTreeEntry* pItem = static_cast<ezQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  return pItem->m_Children.GetCount();
}

int ezQtPropertyAnimModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 1;
}

void ezQtPropertyAnimModel::DocumentStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved2:
    TriggerBuildMapping();
    break;
  }
}

void ezQtPropertyAnimModel::TriggerBuildMapping()
{
  if (m_bBuildMappingQueued)
    return;

  m_bBuildMappingQueued = true;
  QTimer::singleShot(100, this, SLOT(onBuildMappingTriggered()));
}

void ezQtPropertyAnimModel::onBuildMappingTriggered()
{
  BuildMapping();
  m_bBuildMappingQueued = false;
}

void ezQtPropertyAnimModel::BuildMapping()
{
  const ezInt32 iToUse = (m_iInUse + 1) % 2;
  BuildMapping(iToUse);

  if (m_AllEntries[0] != m_AllEntries[1])
  {
    beginResetModel();
    m_iInUse = iToUse;
    endResetModel();
  }
}

void ezQtPropertyAnimModel::BuildMapping(ezInt32 iToUse)
{
  m_TopLevelEntries[iToUse].Clear();
  m_AllEntries[iToUse].Clear();

  const ezPropertyAnimationTrackGroup& group = *m_pAssetDoc->GetProperties();

  ezStringBuilder tmp;

  for (ezUInt32 tIdx = 0; tIdx < group.m_Tracks.GetCount(); ++tIdx)
  {
    ezPropertyAnimationTrack* pTrack = group.m_Tracks[tIdx];

    tmp = pTrack->m_sObjectSearchSequence;
    if (!pTrack->m_sComponentType.IsEmpty())
    {
      tmp.AppendPath(":");
      tmp.Append(pTrack->m_sComponentType.GetData());
    }
    tmp.AppendPath(pTrack->m_sPropertyPath);

    BuildMapping(iToUse, tIdx, pTrack, m_TopLevelEntries[iToUse], -1, tmp);
  }
}

void ezQtPropertyAnimModel::BuildMapping(ezInt32 iToUse, ezInt32 iTrackIdx, ezPropertyAnimationTrack* pTrack, ezDynamicArray<ezInt32>& treeItems, ezInt32 iParentEntry, const char* szPath)
{
  const char* szSubPath = ezStringUtils::FindSubString(szPath, "/");

  ezStringBuilder name, sDisplayString;

  bool bIsComponent = false;
  if (szPath[0] == ':')
  {
    ++szPath;
    bIsComponent = true;
  }

  if (szSubPath != nullptr)
    name.SetSubString_FromTo(szPath, szSubPath);
  else
    name = szPath;

  if (bIsComponent)
    sDisplayString = ezTranslate(name);
  else
    sDisplayString = name;

  ezInt32 iThisEntry = -1;

  for (ezUInt32 i = 0; i < treeItems.GetCount(); ++i)
  {
    if (m_AllEntries[iToUse][treeItems[i]].m_sDisplay.IsEqual_NoCase(sDisplayString))
    {
      iThisEntry = treeItems[i];
      break;
    }
  }

  ezQtPropertyAnimModelTreeEntry* pThisEntry = nullptr;

  if (iThisEntry < 0)
  {
    pThisEntry = &m_AllEntries[iToUse].ExpandAndGetRef();
    iThisEntry = m_AllEntries[iToUse].GetCount() - 1;
    treeItems.PushBack(iThisEntry);

    pThisEntry->m_iParent = iParentEntry;
    pThisEntry->m_uiOwnRowIndex = treeItems.GetCount() - 1;
    pThisEntry->m_sDisplay = sDisplayString;

    if (bIsComponent)
    {
      sDisplayString.Set(":/TypeIcons/", name);
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(sDisplayString);
    }

    if (iParentEntry >= 0)
    {
      ezStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
  else
  {
    pThisEntry = &m_AllEntries[iToUse][iThisEntry];
  }

  if (szSubPath != nullptr)
  {
    szSubPath += 1;
    BuildMapping(iToUse, iTrackIdx, pTrack, pThisEntry->m_Children, iThisEntry, szSubPath);
  }
  else
  {
    pThisEntry->m_iTrackIdx = iTrackIdx;
    pThisEntry->m_pTrack = pTrack;

    switch (pTrack->m_Target)
    {
    case ezPropertyAnimTarget::Color:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/ColorGradient.png");
      break;
    case ezPropertyAnimTarget::Number:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/Curve1D.png");
      break;
    case ezPropertyAnimTarget::VectorX:
    case ezPropertyAnimTarget::RotationX:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveX.png");
      name.Append(".x");
      break;
    case ezPropertyAnimTarget::VectorY:
    case ezPropertyAnimTarget::RotationY:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.png");
      name.Append(".y");
      break;
    case ezPropertyAnimTarget::VectorZ:
    case ezPropertyAnimTarget::RotationZ:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveZ.png");
      name.Append(".z");
      break;
    case ezPropertyAnimTarget::VectorW:
      pThisEntry->m_Icon = ezQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveW.png");
      name.Append(".w");
      break;
    }

    pThisEntry->m_sDisplay = name;

    if (iParentEntry >= 0)
    {
      ezStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
}

