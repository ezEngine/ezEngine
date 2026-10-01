#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/SubGraphNode.h>

namespace
{
  bool IsInputBoundaryNode(const ezRTTI* pType)
  {
    return pType == ezGetStaticRTTI<ezSubGraphTextureInputNode>() || pType == ezGetStaticRTTI<ezSubGraphBufferInputNode>();
  }

  bool IsOutputBoundaryNode(const ezRTTI* pType)
  {
    return pType == ezGetStaticRTTI<ezSubGraphTextureOutputNode>() || pType == ezGetStaticRTTI<ezSubGraphBufferOutputNode>();
  }

  bool IsBoundaryNode(const ezRTTI* pType)
  {
    return IsInputBoundaryNode(pType) || IsOutputBoundaryNode(pType);
  }

  /// Reads one pass or extractor. out_pObject stays null when an editor-only type is not available and was skipped.
  template <typename T>
  ezStatus ImportObject(ezStreamReader& inout_stream, ezTypeVersion uiVersion, ezUniquePtr<T>& out_pObject)
  {
    ezStringBuilder sTypeName;
    bool bEditorOnly = false;
    ezUInt32 uiByteSize = 0;

    inout_stream >> sTypeName;
    if (uiVersion >= 2)
    {
      inout_stream >> bEditorOnly;
      inout_stream >> uiByteSize;
    }

    const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName);
    if (pType == nullptr)
    {
      if (!bEditorOnly)
        return ezStatus(ezFmt("Render pipeline type '{}' is unknown.", sTypeName));

      if (inout_stream.SkipBytes(uiByteSize) != uiByteSize)
        return ezStatus(ezFmt("Failed to skip editor-only render pipeline type '{}'.", sTypeName));

      ezLog::Dev("Skipped editor-only render pipeline type '{}'.", sTypeName);
      return ezStatus(EZ_SUCCESS);
    }

    if (!pType->IsDerivedFrom<T>())
      return ezStatus(ezFmt("Render pipeline type '{}' is not derived from {}.", sTypeName, ezGetStaticRTTI<T>()->GetTypeName()));
    if (pType->GetAllocator() == nullptr || !pType->GetAllocator()->CanAllocate())
      return ezStatus(ezFmt("Render pipeline type '{}' cannot be allocated.", sTypeName));

    out_pObject = pType->GetAllocator()->Allocate<T>();
    if (out_pObject->Deserialize(inout_stream).Failed())
      return ezStatus(ezFmt("Failed to deserialize render pipeline object of type '{}'.", sTypeName));

    return ezStatus(EZ_SUCCESS);
  }

  template <typename T>
  ezResult ExportObject(const T* pObject, ezTypeVersionWriteContext& ref_typeVersionContext, ezStreamWriter& inout_stream)
  {
    const ezRTTI* pType = pObject->GetDynamicRTTI();
    ref_typeVersionContext.AddType(pType);

    // The object is written to a temporary storage first, so that loaders that don't know the type can skip it.
    ezDefaultMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    EZ_SUCCEED_OR_RETURN(pObject->Serialize(writer));

    const bool bEditorOnly = pType->GetAttributeByType<ezRenderPipelineEditorOnlyAttribute>() != nullptr;

    inout_stream << pType->GetTypeName();
    inout_stream << bEditorOnly;
    inout_stream << storage.GetStorageSize32();
    return storage.CopyToStream(inout_stream);
  }
} // namespace

////////////////////////////////////////////////////////////////////////
// ezVisualGraphObjectManager Internal
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineResourceLoaderConnection, ezNoBase, 1, ezRTTIDefaultAllocator<ezRenderPipelineResourceLoaderConnection>)
{
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezRenderPipelineResourceLoaderConnection::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_uiSource;
  inout_stream << m_uiTarget;
  inout_stream << m_sSourcePin;
  inout_stream << m_sTargetPin;

  return EZ_SUCCESS;
}

ezResult ezRenderPipelineResourceLoaderConnection::Deserialize(ezStreamReader& inout_stream)
{
  EZ_VERIFY(ezTypeVersionReadContext::GetContext()->GetTypeVersion(ezGetStaticRTTI<ezRenderPipelineResourceLoaderConnection>()) == 1, "Unknown version");

  inout_stream >> m_uiSource;
  inout_stream >> m_uiTarget;
  inout_stream >> m_sSourcePin;
  inout_stream >> m_sTargetPin;

  return EZ_SUCCESS;
}

// Version 2: Every pass and extractor stores an editor-only flag and its byte size, so that unknown editor-only types can be skipped.
constexpr ezTypeVersion s_RenderPipelineDescriptorVersion = 2;

// static
ezStatus ezRenderPipelineResourceLoader::ImportPipeline(ezStreamReader& ref_streamReader, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& out_passes, ezDynamicArray<ezUniquePtr<ezExtractor>>& out_extractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& out_connections)
{
  out_passes.Clear();
  out_extractors.Clear();
  out_connections.Clear();

  const ezTypeVersion uiVersion = ref_streamReader.ReadVersion(s_RenderPipelineDescriptorVersion);

  ezStringDeduplicationReadContext stringDeduplicationReadContext(ref_streamReader);
  ezTypeVersionReadContext typeVersionReadContext(ref_streamReader);

  // Maps the pass indices in the stream to the indices in out_passes. Skipped passes map to ezInvalidIndex.
  ezDynamicArray<ezUInt32> passRemap;

  // Passes
  {
    ezUInt32 uiNumPasses = 0;
    ref_streamReader >> uiNumPasses;
    out_passes.Reserve(uiNumPasses);
    passRemap.SetCount(uiNumPasses, ezInvalidIndex);

    for (ezUInt32 i = 0; i < uiNumPasses; ++i)
    {
      ezUniquePtr<ezRenderPipelinePass> pPass;
      EZ_SUCCEED_OR_RETURN(ImportObject(ref_streamReader, uiVersion, pPass));

      if (pPass != nullptr)
      {
        passRemap[i] = out_passes.GetCount();
        out_passes.PushBack(std::move(pPass));
      }
    }
  }

  // Extractors
  {
    ezUInt32 uiNumExtractors = 0;
    ref_streamReader >> uiNumExtractors;
    out_extractors.Reserve(uiNumExtractors);

    for (ezUInt32 i = 0; i < uiNumExtractors; ++i)
    {
      ezUniquePtr<ezExtractor> pExtractor;
      EZ_SUCCEED_OR_RETURN(ImportObject(ref_streamReader, uiVersion, pExtractor));

      if (pExtractor != nullptr)
      {
        out_extractors.PushBack(std::move(pExtractor));
      }
    }
  }

  // Connections
  {
    ezUInt32 uiNumConnections = 0;
    ref_streamReader >> uiNumConnections;
    out_connections.Reserve(uiNumConnections);

    ezRenderPipelineResourceLoaderConnection connection;
    for (ezUInt32 i = 0; i < uiNumConnections; ++i)
    {
      if (connection.Deserialize(ref_streamReader).Failed())
        return ezStatus(ezFmt("Failed to deserialize render pipeline connection {}.", i));

      if (connection.m_uiSource >= passRemap.GetCount() || connection.m_uiTarget >= passRemap.GetCount())
        return ezStatus(ezFmt("Render pipeline connection {} references a pass index outside of the {} passes in the pipeline.", i, passRemap.GetCount()));

      connection.m_uiSource = passRemap[connection.m_uiSource];
      connection.m_uiTarget = passRemap[connection.m_uiTarget];

      // Drop connections to skipped editor-only passes.
      if (connection.m_uiSource == ezInvalidIndex || connection.m_uiTarget == ezInvalidIndex)
        continue;

      out_connections.PushBack(connection);
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRenderPipelineResourceLoader::InlineImportedSubGraphs(ezDynamicArray<ezRenderPipelineNode*>& ref_nodes, ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>& ref_ownedPasses, ezDynamicArray<ezExtractor*>& ref_extractors, ezDynamicArray<ezUniquePtr<ezExtractor>>& ref_ownedExtractors, ezDynamicArray<ezRenderPipelineResourceLoaderConnection>& ref_connections, const ImportPipelineCallback& importPipeline)
{
  ezSet<const ezRTTI*> extractorTypes;
  for (const ezExtractor* pExtractor : ref_extractors)
  {
    extractorTypes.Insert(pExtractor->GetDynamicRTTI());
  }

  for (ezUInt32 iSub = 0; iSub < ref_nodes.GetCount(); ++iSub)
  {
    const ezSubGraphNode* pSubGraph = ezDynamicCast<const ezSubGraphNode*>(ref_nodes[iSub]);
    if (pSubGraph == nullptr)
      continue;

    ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> importedPasses;
    ezDynamicArray<ezUniquePtr<ezExtractor>> importedExtractors;
    ezDynamicArray<ezRenderPipelineResourceLoaderConnection> importedConnections;

    // Sub-pipeline binaries are already fully inlined because dependencies are transformed first.
    // Therefore, no imported pipeline can contain another Subgraph node at this point.
    ezStatus res = importPipeline(pSubGraph->m_sPipeline, importedPasses, importedExtractors, importedConnections);
    if (res.Failed())
      return ezStatus(ezFmt("Failed to import sub-graph pipeline '{}': {}", pSubGraph->m_sPipeline, res.GetMessageString()));

    for (const ezRenderPipelineResourceLoaderConnection& connection : importedConnections)
    {
      if (connection.m_uiSource >= importedPasses.GetCount() || connection.m_uiTarget >= importedPasses.GetCount())
        return ezStatus(ezFmt("Sub-graph pipeline '{}' contains a connection with an invalid node index.", pSubGraph->m_sPipeline));
    }

    // Map sub-graph pass indices to parent pass indices. Boundary nodes are eliminated during
    // inlining and retain ezInvalidIndex in this mapping.
    ezDynamicArray<ezUInt32> subToParentIndex;
    subToParentIndex.SetCount(importedPasses.GetCount(), ezInvalidIndex);
    for (ezUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
    {
      ezRenderPipelinePass* pPass = importedPasses[iSubNode].Borrow();
      if (IsBoundaryNode(pPass->GetDynamicRTTI()))
        continue;

      subToParentIndex[iSubNode] = ref_nodes.GetCount();
      ref_nodes.PushBack(pPass);
    }

    // Add internal sub-graph connections between non-boundary passes.
    for (const ezRenderPipelineResourceLoaderConnection& subConn : importedConnections)
    {
      const bool bSourceIsBoundary = IsBoundaryNode(importedPasses[subConn.m_uiSource]->GetDynamicRTTI());
      const bool bTargetIsBoundary = IsBoundaryNode(importedPasses[subConn.m_uiTarget]->GetDynamicRTTI());
      if (bSourceIsBoundary || bTargetIsBoundary)
        continue;

      ezRenderPipelineResourceLoaderConnection& newConn = ref_connections.ExpandAndGetRef();
      newConn = subConn;
      newConn.m_uiSource = subToParentIndex[subConn.m_uiSource];
      newConn.m_uiTarget = subToParentIndex[subConn.m_uiTarget];
    }

    struct BoundaryPassthrough
    {
      ezString m_sOutput;
      ezUInt32 m_uiSource = ezInvalidIndex;
      ezString m_sSourcePin;
    };

    // Find connections that connect an input directly to an output node (i.e. direct passthrough)
    ezDynamicArray<BoundaryPassthrough> boundaryPassthroughs;
    for (const ezRenderPipelineResourceLoaderConnection& subConn : importedConnections)
    {
      ezRenderPipelinePass* pInputBoundary = importedPasses[subConn.m_uiSource].Borrow();
      ezRenderPipelinePass* pOutputBoundary = importedPasses[subConn.m_uiTarget].Borrow();
      if (!IsInputBoundaryNode(pInputBoundary->GetDynamicRTTI()) || !IsOutputBoundaryNode(pOutputBoundary->GetDynamicRTTI()))
        continue;

      BoundaryPassthrough& forward = boundaryPassthroughs.ExpandAndGetRef();
      forward.m_sOutput = pOutputBoundary->GetName();

      for (const ezRenderPipelineResourceLoaderConnection& parentConn : ref_connections)
      {
        if (parentConn.m_uiTarget == iSub && parentConn.m_sTargetPin == pInputBoundary->GetName())
        {
          forward.m_uiSource = parentConn.m_uiSource;
          forward.m_sSourcePin = parentConn.m_sSourcePin;
          break;
        }
      }
    }

    // Remap connections to SubGraph input pins to every internal consumer of the matching input
    // boundary. Iterate only over existing parent connections because fan-out adds new entries.
    const ezUInt32 uiConnCountBeforeInputRemap = ref_connections.GetCount();
    for (ezUInt32 iConn = 0; iConn < uiConnCountBeforeInputRemap; ++iConn)
    {
      if (ref_connections[iConn].m_uiTarget != iSub)
        continue;

      const ezString sPinName = ref_connections[iConn].m_sTargetPin;
      bool bPinFound = false;
      bool bRemapped = false;
      for (ezUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
      {
        ezRenderPipelinePass* pBoundary = importedPasses[iSubNode].Borrow();
        if (!IsInputBoundaryNode(pBoundary->GetDynamicRTTI()) || pBoundary->GetName() != sPinName)
          continue;

        bPinFound = true;
        for (const ezRenderPipelineResourceLoaderConnection& subConn : importedConnections)
        {
          if (subConn.m_uiSource != iSubNode || subConn.m_sSourcePin != "Value")
            continue;
          if (subToParentIndex[subConn.m_uiTarget] == ezInvalidIndex)
            continue;

          if (!bRemapped)
          {
            ref_connections[iConn].m_uiTarget = subToParentIndex[subConn.m_uiTarget];
            ref_connections[iConn].m_sTargetPin = subConn.m_sTargetPin;
            bRemapped = true;
          }
          else
          {
            ezRenderPipelineResourceLoaderConnection extra = ref_connections[iConn];
            extra.m_uiTarget = subToParentIndex[subConn.m_uiTarget];
            extra.m_sTargetPin = subConn.m_sTargetPin;
            ref_connections.PushBack(std::move(extra));
          }
        }
        break;
      }

      if (!bPinFound)
        return ezStatus(ezFmt("Sub-graph '{}' no longer has an input pin named '{}'.", pSubGraph->m_sPipeline, sPinName));
      if (!bRemapped)
        ref_connections[iConn].m_uiSource = ezInvalidIndex;
    }

    // Remap connections from SubGraph output pins to the internal producer of the matching output
    // boundary. Direct input-to-output connections use the captured parent input endpoint.
    for (ezRenderPipelineResourceLoaderConnection& parentConn : ref_connections)
    {
      if (parentConn.m_uiSource != iSub)
        continue;

      const ezString sPinName = parentConn.m_sSourcePin;
      bool bPinFound = false;
      bool bRemapped = false;
      for (ezUInt32 iSubNode = 0; iSubNode < importedPasses.GetCount(); ++iSubNode)
      {
        ezRenderPipelinePass* pBoundary = importedPasses[iSubNode].Borrow();
        if (!IsOutputBoundaryNode(pBoundary->GetDynamicRTTI()) || pBoundary->GetName() != sPinName)
          continue;

        bPinFound = true;
        for (const ezRenderPipelineResourceLoaderConnection& subConn : importedConnections)
        {
          if (subConn.m_uiTarget != iSubNode || subConn.m_sTargetPin != "Value")
            continue;
          if (subToParentIndex[subConn.m_uiSource] == ezInvalidIndex)
          {
            for (const BoundaryPassthrough& forward : boundaryPassthroughs)
            {
              if (forward.m_sOutput == sPinName)
              {
                parentConn.m_uiSource = forward.m_uiSource;
                parentConn.m_sSourcePin = forward.m_sSourcePin;
                bRemapped = forward.m_uiSource != ezInvalidIndex;
                break;
              }
            }
            break;
          }

          parentConn.m_uiSource = subToParentIndex[subConn.m_uiSource];
          parentConn.m_sSourcePin = subConn.m_sSourcePin;
          bRemapped = true;
          break;
        }
        break;
      }

      if (!bPinFound)
        return ezStatus(ezFmt("Sub-graph '{}' no longer has an output pin named '{}'.", pSubGraph->m_sPipeline, sPinName));
      if (!bRemapped)
        parentConn.m_uiSource = ezInvalidIndex;
    }

    // Parent extractors override imported extractors of the same type.
    for (ezUniquePtr<ezExtractor>& pExtractor : importedExtractors)
    {
      if (extractorTypes.Contains(pExtractor->GetDynamicRTTI()))
        continue;

      extractorTypes.Insert(pExtractor->GetDynamicRTTI());
      ref_extractors.PushBack(pExtractor.Borrow());
      ref_ownedExtractors.PushBack(std::move(pExtractor));
    }

    // Keep imported passes alive until the flattened pipeline has been serialized.
    for (ezUniquePtr<ezRenderPipelinePass>& pPass : importedPasses)
    {
      ref_ownedPasses.PushBack(std::move(pPass));
    }

    // Remove connections to unconnected boundaries.
    for (ezUInt32 iConn = ref_connections.GetCount(); iConn-- > 0;)
    {
      if (ref_connections[iConn].m_uiSource == ezInvalidIndex || ref_connections[iConn].m_uiTarget == ezInvalidIndex)
        ref_connections.RemoveAtAndCopy(iConn);
    }

    // Remove the SubGraph placeholder and update indices shifted by RemoveAtAndCopy.
    ref_nodes.RemoveAtAndCopy(iSub);
    for (ezRenderPipelineResourceLoaderConnection& conn : ref_connections)
    {
      if (conn.m_uiSource > iSub)
        --conn.m_uiSource;
      if (conn.m_uiTarget > iSub)
        --conn.m_uiTarget;
    }

    --iSub;
  }

  return ezStatus(EZ_SUCCESS);
}

// static
ezInternal::NewInstance<ezRenderPipeline> ezRenderPipelineResourceLoader::CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc)
{
  ezRawMemoryStreamReader stream(desc.m_SerializedPipeline);
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> passes;
  ezDynamicArray<ezUniquePtr<ezExtractor>> extractors;
  ezDynamicArray<ezRenderPipelineResourceLoaderConnection> connections;
  const ezStatus res = ImportPipeline(stream, passes, extractors, connections);
  if (res.Failed())
  {
    ezLog::Error("Failed to import render pipeline '{}': {}", desc.m_sPath, res.GetMessageString());
    return nullptr;
  }

  return EZ_DEFAULT_NEW(ezRenderPipeline, std::move(passes), std::move(extractors), connections.GetArrayPtr());
}

ezResult ezRenderPipelineResourceLoader::ExportPipeline(ezArrayPtr<const ezRenderPipelinePass* const> passes, ezArrayPtr<const ezExtractor* const> extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections, ezStreamWriter& ref_streamWriter)
{
  ref_streamWriter.WriteVersion(s_RenderPipelineDescriptorVersion);

  ezStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_streamWriter);
  ezTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  // passes
  {
    const ezUInt32 uiNumPasses = passes.GetCount();
    stream << uiNumPasses;

    for (auto& pass : passes)
    {
      EZ_SUCCEED_OR_RETURN(ExportObject(pass, typeVersionWriteContext, stream));
    }
  }

  // extractors
  {
    const ezUInt32 uiNumExtractors = extractors.GetCount();
    stream << uiNumExtractors;

    for (auto& extractor : extractors)
    {
      EZ_SUCCEED_OR_RETURN(ExportObject(extractor, typeVersionWriteContext, stream));
    }
  }

  // Connections
  {
    const ezUInt32 uiNumConnections = connections.GetCount();
    stream << uiNumConnections;

    typeVersionWriteContext.AddType(ezGetStaticRTTI<ezRenderPipelineResourceLoaderConnection>());

    for (auto& connection : connections)
    {
      EZ_SUCCEED_OR_RETURN(connection.Serialize(stream));
    }
  }

  EZ_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  EZ_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
