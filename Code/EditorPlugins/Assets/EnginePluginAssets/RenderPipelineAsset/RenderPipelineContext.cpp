#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineContextLoaderConnection, ezNoBase, 1, ezRTTIDefaultAllocator<ezRenderPipelineContextLoaderConnection>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Connection::Source", m_Source),
    EZ_MEMBER_PROPERTY("Connection::Target", m_Target),
    EZ_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),    
    EZ_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on


const ezRTTI* ezRenderPipelineRttiConverterContext::FindTypeByName(ezStringView sName) const
{
  if (sName == "DocumentNodeManager_DefaultConnection")
  {
    return ezGetStaticRTTI<ezRenderPipelineContextLoaderConnection>();
  }
  return ezWorldRttiConverterContext::FindTypeByName(sName);
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineContext, 1, ezRTTIDefaultAllocator<ezRenderPipelineContext>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_CONSTANT_PROPERTY("DocumentType", (const char*) "RenderPipeline"),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRenderPipelineContext::ezRenderPipelineContext()
  : ezEngineProcessDocumentContext(ezEngineProcessDocumentContextFlags::CreateWorld)
{
}

void ezRenderPipelineContext::HandleMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  ezEngineProcessDocumentContext::HandleMessage(pMsg);
}

void ezRenderPipelineContext::OnInitialize()
{
}

ezEngineProcessViewContext* ezRenderPipelineContext::CreateViewContext()
{
  EZ_ASSERT_DEV(false, "Should not be called");
  return nullptr;
}

void ezRenderPipelineContext::DestroyViewContext(ezEngineProcessViewContext* pContext)
{
  EZ_ASSERT_DEV(false, "Should not be called");
}

ezWorldRttiConverterContext& ezRenderPipelineContext::GetContext()
{
  return m_RenderPipelineContext;
}

const ezWorldRttiConverterContext& ezRenderPipelineContext::GetContext() const
{
  return m_RenderPipelineContext;
}

ezStatus ezRenderPipelineContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezDynamicArray<ezRenderPipelinePass*> passes;
  ezDynamicArray<ezUuid> passUuids;
  ezDynamicArray<ezExtractor*> extractors;
  ezDynamicArray<ezRenderPipelineContextLoaderConnection*> connections;

  m_RenderPipelineContext.GetObjectsByType(passes, &passUuids);
  m_RenderPipelineContext.GetObjectsByType(extractors);
  m_RenderPipelineContext.GetObjectsByType(connections);

  ezHashTable<ezUuid, ezUInt32> passUuidToIndex;
  for(ezUInt32 i=0; i < passUuids.GetCount(); ++i)
  {
    passUuidToIndex.Insert(passUuids[i], i);
  }


  ezStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_originalStream);
  ezTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  // passes
  {
    const ezUInt32 uiNumPasses = passes.GetCount();
    stream << uiNumPasses;

    for (auto& pass: passes)
    {
      auto pPassType = pass->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pPassType);

      stream << pPassType->GetTypeName();
      EZ_SUCCEED_OR_RETURN(pass->Serialize(stream));
    }
  }

  // extractors
  {
    const ezUInt32 uiNumExtractors = extractors.GetCount();
    stream << uiNumExtractors;

    for (auto& extractor : extractors)
    {
      auto pExtractorType = pass->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pExtractorType);

      stream << pExtractorType->GetTypeName();
      EZ_SUCCEED_OR_RETURN(extractor->Serialize(stream));
    }
  }

  // Connections
  {
    const ezUInt32 uiNumConnections = connections.GetCount();
    stream << uiNumConnections;

    typeVersionWriteContext.AddType(ezGetStaticRTTI<ezRenderPipelineResourceLoaderConnection>());

    for (auto& connection : connections)
    {
      ezRenderPipelineResourceLoaderConnection engineConnection;
      EZ_VERIFY(passUuidsToIndex.TryGetValue(connection.m_Source, engineConnection.m_uiSource));
      EZ_VERIFY(passUuidToIndex.TryGetValue(connection.m_Target, engineConnection.m_uiTarget));
      engineConnection.m_sSourcePin = connection.m_SourcePin;
      engineConnection.m_sTargetPin = connection.m_TargetPin;

      EZ_SUCCEED_OR_RETURN(engineConnection.Serialize(stream));
    }
  }

  return ezStatus(EZ_SUCCESS);
}