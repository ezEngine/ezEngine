#include <PCH.h>
#include <EditorPluginScene/DragDropHandlers/CollisionMeshDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezCollisionMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE


float ezCollisionMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return (IsSpecificAssetType(pInfo, "Collision Mesh") || IsSpecificAssetType(pInfo, "Collision Mesh (Convex)")) ? 1.0f : 0.0f;
}

void ezCollisionMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "ezPxStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), ezUuid(), -1);
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("ezPxStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
      CreateDropObject(pInfo->m_vDropPosition, "ezPxStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
