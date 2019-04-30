#include <EnginePluginProceduralPlacementPCH.h>

#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EnginePluginProceduralPlacement/ProceduralPlacementAsset/ProceduralPlacementContext.h>
#include <SharedPluginAssets/Common/Messages.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProceduralPlacementContext, 1, ezRTTIDefaultAllocator<ezProceduralPlacementContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "Procedural Placement Asset"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezProceduralPlacementContext::ezProceduralPlacementContext() {}

void ezProceduralPlacementContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCreateThumbnailMsgToEngine>())
  {
    ezResourceManager::RestoreResource(m_hProcGen);
  }

  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezProceduralPlacementContext::OnInitialize()
{
  ezStringBuilder sProcGenGuid;
  ezConversionUtils::ToString(GetDocumentGuid(), sProcGenGuid);
  m_hProcGen = ezResourceManager::LoadResource<ezProceduralPlacementResource>(sProcGenGuid);
}

ezEngineProcessViewContext* ezProceduralPlacementContext::CreateViewContext()
{
  return nullptr;
}

void ezProceduralPlacementContext::DestroyViewContext(ezEngineProcessViewContext* pContext) {}
