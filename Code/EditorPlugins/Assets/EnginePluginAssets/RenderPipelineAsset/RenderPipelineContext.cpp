#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

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

ezStatus ezRenderPipelineContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezDynamicArray<ezRenderPipelinePass*> passes;
  ezDynamicArray<ezExtractor*> extractors;
  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;

  ezDynamicArray<ezUuid> passUuids;
  ezDynamicArray<ezDocumentObject_ConnectionBase*> toolConnections;

  m_Context.GetObjectsByType(passes, &passUuids);
  m_Context.GetObjectsByType(extractors);
  m_Context.GetObjectsByType(toolConnections);

  ezHashTable<ezUuid, ezUInt32> passUuidToIndex;
  for (ezUInt32 i = 0; i < passUuids.GetCount(); ++i)
  {
    passUuidToIndex.Insert(passUuids[i], i);
  }
  connections.SetCount(toolConnections.GetCount());
  for (ezUInt32 i = 0; i < toolConnections.GetCount(); i++)
  {
    ezDocumentObject_ConnectionBase* pConnection = toolConnections[i];
    ezRenderPipelineResourceLoaderConnection& engineConnection = connections[i];
    EZ_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Source, engineConnection.m_uiSource), "");
    EZ_VERIFY(passUuidToIndex.TryGetValue(pConnection->m_Target, engineConnection.m_uiTarget), "");
    engineConnection.m_sSourcePin = pConnection->m_SourcePin;
    engineConnection.m_sTargetPin = pConnection->m_TargetPin;
  }

  ezDefaultMemoryStreamStorage storage;
  {
    // Export Resource Data
    ezMemoryStreamWriter writer(&storage);
    EZ_SUCCEED_OR_RETURN(ezRenderPipelineResourceLoader::ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), writer));
  }

  ezDeferredFileWriter file;
  file.SetOutput(pMsg->m_sOutputFile);

  {
    // File Header
    ezAssetFileHeader header;
    header.SetFileHashAndVersion(pMsg->m_uiAssetHash, pMsg->m_uiVersion);
    header.Write(file).IgnoreResult();

    ezUInt8 uiVersion = 2;
    file << uiVersion;
  }

  {
    // Resource Data
    ezUInt32 uiSize = storage.GetStorageSize32();
    file << uiSize;
    if (storage.CopyToStream(file).Failed())
      return ezStatus(ezFmt("Failed to copy content to file writer for '{}'", pMsg->m_sOutputFile));
  }

  // do the actual file writing
  if (file.Close().Failed())
    return ezStatus(ezFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return ezStatus(EZ_SUCCESS);
}
