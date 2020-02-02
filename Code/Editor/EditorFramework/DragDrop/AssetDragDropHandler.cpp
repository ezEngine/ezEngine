#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

#include <QDataStream>
#include <QMimeData>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDragDropHandler, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

bool ezAssetDragDropHandler::IsAssetType(const ezDragDropInfo* pInfo) const
{
  return pInfo->m_pMimeData->hasFormat("application/ezEditor.AssetGuid");
}

ezString ezAssetDragDropHandler::GetAssetGuidString(const ezDragDropInfo* pInfo) const
{
  QByteArray ba = pInfo->m_pMimeData->data("application/ezEditor.AssetGuid");
  QDataStream stream(&ba, QIODevice::ReadOnly);

  int iGuids = 0;
  stream >> iGuids;

  if (iGuids > 1)
  {
    ezLog::Warning("Dragging more than one asset type is currently not supported");
  }

  QString sGuid;
  stream >> sGuid;

  return sGuid.toUtf8().data();
}

ezString ezAssetDragDropHandler::GetAssetsDocumentTypeName(const ezUuid& assetTypeGuid) const
{
  return ezAssetCurator::GetSingleton()->GetSubAsset(assetTypeGuid)->m_Data.m_sSubAssetsDocumentTypeName.GetData();
}

bool ezAssetDragDropHandler::IsSpecificAssetType(const ezDragDropInfo* pInfo, const char* szType) const
{
  if (!IsAssetType(pInfo))
    return false;

  const ezUuid guid = GetAssetGuid(pInfo);

  return GetAssetsDocumentTypeName(guid) == szType;
}
