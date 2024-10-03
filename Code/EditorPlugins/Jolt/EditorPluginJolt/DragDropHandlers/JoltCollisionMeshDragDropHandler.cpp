#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginJolt/DragDropHandlers/JoltCollisionMeshDragDropHandler.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshComponentDragDropHandler, 1, ezRTTIDefaultAllocator<ezJoltCollisionMeshComponentDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;


float ezJoltCollisionMeshComponentDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (ezComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return (IsSpecificAssetType(pInfo, "Jolt_Colmesh_Triangle") || IsSpecificAssetType(pInfo, "Jolt_Colmesh_Convex")) ? 1.0f : 0.0f;
}

void ezJoltCollisionMeshComponentDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  ezComponentDragDropHandler::OnDragBegin(pInfo);

  constexpr const char* szComponentType = "ezJoltStaticActorComponent";
  constexpr const char* szPropertyName = "CollisionMesh";

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_ActiveParentObject, -1);
  }
  else
  {
    if (!pInfo->m_bCtrlKeyDown && pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject(szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, szComponentType, szPropertyName, GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
