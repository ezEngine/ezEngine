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
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineResourceUpdateMsg>())
  {
    const ezEditorEngineResourceUpdateMsg* pMsg2 = static_cast<const ezEditorEngineResourceUpdateMsg*>(pMsg);

    if (pMsg2->m_sResourceType == "ProceduralPlacement")
    {
      ezUniquePtr<ezResourceLoaderFromMemory> loader(EZ_DEFAULT_NEW(ezResourceLoaderFromMemory));
      loader->m_ModificationTimestamp = ezTimestamp::CurrentTimestamp();
      loader->m_sResourceDescription = "ProcGenImmediateEditorUpdate";
      ezMemoryStreamWriter memoryWriter(&loader->m_CustomData);
      memoryWriter.WriteBytes(pMsg2->m_Data.GetData(), pMsg2->m_Data.GetCount());

      ezResourceManager::UpdateResourceWithCustomLoader(m_hProcGen, std::move(loader));

      // force loading of the resource
      ezResourceLock<ezProceduralPlacementResource> pResource(m_hProcGen, ezResourceAcquireMode::NoFallback);
    }
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineRestoreResourceMsg>() ||
      pMsg->GetDynamicRTTI()->IsDerivedFrom<ezCreateThumbnailMsgToEngine>())
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
