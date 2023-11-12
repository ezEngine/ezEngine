#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DragDrop/AssetDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>


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

  ezHybridArray<QString, 1> guids;
  stream >> guids;

  if (guids.GetCount() > 1)
  {
    ezLog::Warning("Dragging more than one asset type is currently not supported");
  }

  return guids[0].toUtf8().data();
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
