#include <PCH.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE


float ezMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Mesh") ? 1.0f : 0.0f;
}

void ezMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  CreateDropObject(pInfo->m_vDropPosition, "ezMeshComponent", "Mesh", GetAssetGuidString(pInfo));
  SelectCreatedObjects();
  BeginTemporaryCommands();
}
