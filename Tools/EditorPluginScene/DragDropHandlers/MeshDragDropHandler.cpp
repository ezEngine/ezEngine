#include <PCH.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE


bool ezMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  return IsSpecificAssetType(pInfo, "Mesh");
}

void ezMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  CreateDropObject(pInfo->m_vDropPosition, "ezMeshComponent", "Mesh", GetAssetGuidString(pInfo));
  SelectCreatedObjects();
  BeginTemporaryCommands();
}
