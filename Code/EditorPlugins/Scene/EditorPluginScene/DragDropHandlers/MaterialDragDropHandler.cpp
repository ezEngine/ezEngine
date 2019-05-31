#include <EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/DocumentManager.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialDragDropHandler, 1, ezRTTIDefaultAllocator<ezMaterialDragDropHandler>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezMaterialDragDropHandler::RequestConfiguration(ezDragDropConfig* pConfigToFillOut)
{
  pConfigToFillOut->m_bPickSelectedObjects = true;
}

float ezMaterialDragDropHandler::CanHandle(const ezDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport")
    return 0.0f;

  const ezDocument* pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom<ezSceneDocument>())
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Material") ? 1.0f : 0.0f;
}

void ezMaterialDragDropHandler::OnDragBegin(const ezDragDropInfo* pInfo)
{
  m_pDocument = ezDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  EZ_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
}

void ezMaterialDragDropHandler::OnDragUpdate(const ezDragDropInfo* pInfo)
{
  if (!pInfo->m_TargetComponent.IsValid())
    return;

  const ezDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

  if (!pComponent)
    return;

  if (m_AppliedToComponent == pInfo->m_TargetComponent && m_uiAppliedToSlot == pInfo->m_iTargetObjectSubID)
    return;

  m_AppliedToComponent = pInfo->m_TargetComponent;
  m_uiAppliedToSlot = pInfo->m_iTargetObjectSubID;

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezMeshComponent>())
  {
    ezResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_Index = pInfo->m_iTargetObjectSubID;
    cmd.m_sProperty = "Materials";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd);
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGreyBoxComponent>())
  {
    ezSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_sProperty = "Material";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd);
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }
}

void ezMaterialDragDropHandler::OnDragCancel()
{
  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void ezMaterialDragDropHandler::OnDrop(const ezDragDropInfo* pInfo)
{
  if (pInfo->m_TargetComponent.IsValid())
  {
    const ezDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

    if (pComponent && (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezMeshComponent>() ||
                        pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<ezGreyBoxComponent>()))
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
      return;
    }
  }

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}
