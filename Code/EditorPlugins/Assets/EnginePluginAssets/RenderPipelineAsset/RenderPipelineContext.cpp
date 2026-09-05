#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>
#include <EnginePluginAssets/RenderPipelineAsset/RenderPipelineContext.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

/// Version of the file that wraps the serialized pipeline. Has to match the version that ezRenderPipelineResource expects.
static constexpr ezUInt8 s_uiBinRenderPipelineVersion = 2;

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

/// Loads a binary render pipeline from an asset GUID string (resolving via the asset redirection table).
static ezStatus ImportSubPipeline(ezStringView sGuidOrPath, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& out_passes, ezDynamicArray<ezUniquePtr<ezExtractor>>& out_extractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& out_connections)
{
  ezFileReader file;
  if (file.Open(sGuidOrPath).Failed())
    return ezStatus(ezFmt("Failed to open render pipeline '{}'.", sGuidOrPath));

  ezAssetFileHeader header;
  if (header.Read(file).Failed())
    return ezStatus(ezFmt("Render pipeline '{}' has an invalid asset header.", sGuidOrPath));

  ezUInt8 uiVersion = 0;
  file >> uiVersion;
  if (uiVersion != s_uiBinRenderPipelineVersion)
    return ezStatus(ezFmt("Render pipeline '{}' has version {}, expected {}.", sGuidOrPath, uiVersion, s_uiBinRenderPipelineVersion));

  ezUInt32 uiSize = 0;
  file >> uiSize;
  EZ_IGNORE_UNUSED(uiSize);

  // The pipeline data follows directly; deserialize it from the file stream.
  return ezRenderPipelineResourceLoader::ImportPipeline(file, out_passes, out_extractors, out_connections);
}

ezStatus ezRenderPipelineContext::ExportDocument(const ezExportDocumentMsgToEngine* pMsg)
{
  ezDynamicArray<ezRenderPipelineNode*> nodes;
  ezDynamicArray<ezUuid> nodeUuids;
  ezDynamicArray<ezExtractor*> extractors;
  ezDynamicArray<ezDocumentObject_ConnectionBase*> toolConnections;
  m_Context.GetObjectsByType(nodes, &nodeUuids);
  m_Context.GetObjectsByType(extractors);
  m_Context.GetObjectsByType(toolConnections);

  // Build the UUID-to-index map used by serialized connections.
  ezHashTable<ezUuid, ezUInt32> uuidToIndex;
  for (ezUInt32 i = 0; i < nodes.GetCount(); ++i)
  {
    uuidToIndex.Insert(nodeUuids[i], i);
    if (ezDynamicCast<ezRenderPipelinePass*>(nodes[i]) == nullptr && ezDynamicCast<ezSubGraphNode*>(nodes[i]) == nullptr)
      return ezStatus(ezFmt("Unsupported GPU pipeline node type '{}'.", nodes[i]->GetDynamicRTTI()->GetTypeName()));
  }

  // Validate that the root graph contains at most one extractor of each type. Imported extractors
  // with matching types are ignored later so root extractors take precedence.
  ezSet<const ezRTTI*> extractorTypes;
  for (const ezExtractor* pExtractor : extractors)
  {
    const ezRTTI* pType = pExtractor->GetDynamicRTTI();
    if (extractorTypes.Contains(pType))
      return ezStatus(ezFmt("The pipeline contains more than one extractor of type '{}'.", pType->GetTypeName()));
    extractorTypes.Insert(pType);
  }

  // Convert tool connection UUIDs to indices in the temporary compiled node list.
  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  connections.Reserve(toolConnections.GetCount());
  for (const ezDocumentObject_ConnectionBase* pConn : toolConnections)
  {
    auto& conn = connections.ExpandAndGetRef();
    if (!uuidToIndex.TryGetValue(pConn->m_Source, conn.m_uiSource))
      return ezStatus(ezFmt("Connection source UUID '{}' was not found in the pipeline node list.", pConn->m_Source));
    if (!uuidToIndex.TryGetValue(pConn->m_Target, conn.m_uiTarget))
      return ezStatus(ezFmt("Connection target UUID '{}' was not found in the pipeline node list.", pConn->m_Target));
    conn.m_sSourcePin = pConn->m_SourcePin;
    conn.m_sTargetPin = pConn->m_TargetPin;
  }

  // Inline all SubGraph placeholders by importing their already transformed render pipeline data.
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> ownedPasses;
  ezDynamicArray<ezUniquePtr<ezExtractor>> ownedExtractors;
  EZ_SUCCEED_OR_RETURN(ezRenderPipelineResourceLoader::InlineImportedSubGraphs(nodes, ownedPasses, extractors, ownedExtractors, connections, ImportSubPipeline));

  ezDynamicArray<ezRenderPipelinePass*> passes;
  passes.Reserve(nodes.GetCount());
  for (ezRenderPipelineNode* pNode : nodes)
  {
    ezRenderPipelinePass* pPass = ezDynamicCast<ezRenderPipelinePass*>(pNode);
    if (pPass == nullptr)
      return ezStatus("Failed to inline all GPU pipeline sub-graphs.");
    passes.PushBack(pPass);
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
    header.Write(file).AssertSuccess();

    file << s_uiBinRenderPipelineVersion;
  }

  {
    // Resource Data
    ezUInt32 uiSize = storage.GetStorageSize32();
    file << uiSize;
    if (storage.CopyToStream(file).Failed())
      return ezStatus(ezFmt("Failed to copy pipeline data to '{}'.", pMsg->m_sOutputFile));
  }

  // do the actual file writing
  if (file.Close().Failed())
    return ezStatus(ezFmt("Writing to '{}' failed.", pMsg->m_sOutputFile));

  return ezStatus(EZ_SUCCESS);
}
