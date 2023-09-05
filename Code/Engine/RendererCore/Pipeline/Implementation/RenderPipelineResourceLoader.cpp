#include <RendererCore/RendererCorePCH.h>

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

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager Internal
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

constexpr ezTypeVersion s_RenderPipelineDescriptorVersion = 1;

// static
ezInternal::NewInstance<ezRenderPipeline> ezRenderPipelineResourceLoader::CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);

  ezRawMemoryStreamReader inout_stream(desc.m_SerializedPipeline);

  const auto uiVersion = inout_stream.ReadVersion(s_RenderPipelineDescriptorVersion);
  EZ_IGNORE_UNUSED(uiVersion);

  ezStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  ezTypeVersionReadContext typeVersionReadContext(inout_stream);

  ezStringBuilder sTypeName;

  ezHybridArray<ezRenderPipelinePass*, 16> passes;

  // Passes
  {
    ezUInt32 uiNumPasses = 0;
    inout_stream >> uiNumPasses;

    for (ezUInt32 i = 0; i < uiNumPasses; ++i)
    {
      inout_stream >> sTypeName;
      if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezRenderPipelinePass> pPass = pType->GetAllocator()->Allocate<ezRenderPipelinePass>();
        pPass->Deserialize(inout_stream).AssertSuccess("");
        passes.PushBack(pPass.Borrow());
        pPipeline->AddPass(std::move(pPass));
      }
      else
      {
        ezLog::Error("Unknown render pipeline pass type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Extractors
  {
    ezUInt32 uiNumExtractors = 0;
    inout_stream >> uiNumExtractors;

    for (ezUInt32 i = 0; i < uiNumExtractors; ++i)
    {
      inout_stream >> sTypeName;
      if (const ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName))
      {
        ezUniquePtr<ezExtractor> pExtractor = pType->GetAllocator()->Allocate<ezExtractor>();
        pExtractor->Deserialize(inout_stream).AssertSuccess("");
        pPipeline->AddExtractor(std::move(pExtractor));
      }
      else
      {
        ezLog::Error("Unknown render pipeline extractor type '{}'", sTypeName);
        return nullptr;
      }
    }
  }

  // Connections
  {
    ezUInt32 uiNumConnections = 0;
    inout_stream >> uiNumConnections;

    for (ezUInt32 i = 0; i < uiNumConnections; ++i)
    {
      ezRenderPipelineResourceLoaderConnection data;
      data.Deserialize(inout_stream).AssertSuccess("Failed to deserialize render pipeline connection");

      ezRenderPipelinePass* pSource = passes[data.m_uiSource];
      ezRenderPipelinePass* pTarget = passes[data.m_uiTarget];

      if (!pPipeline->Connect(pSource, data.m_sSourcePin, pTarget, data.m_sTargetPin))
      {
        ezLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_sSourcePin, pTarget->GetName(), data.m_sTargetPin);
      }
    }
  }
  return pPipeline;
}

// static
void ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& ref_desc)
{
  ezHybridArray<const ezRenderPipelinePass*, 16> passes;
  ezHybridArray<const ezExtractor*, 16> extractors;
  ezHybridArray<ezRenderPipelineResourceLoaderConnection, 16> connections;

  ezHashTable<const ezRenderPipelineNode*, ezUInt32> passToIndex;
  pPipeline->GetPasses(passes);
  pPipeline->GetExtractors(extractors);

  passToIndex.Reserve(passes.GetCount());
  for (ezUInt32 i = 0; i < passes.GetCount(); i++)
  {
    passToIndex.Insert(passes[i], i);
  }


  for (ezUInt32 i = 0; i < passes.GetCount(); i++)
  {
    const ezRenderPipelinePass* pSource = passes[i];

    ezRenderPipelineResourceLoaderConnection data;
    data.m_uiSource = i;

    auto outputs = pSource->GetOutputPins();
    for (const ezRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_sSourcePin = pSource->GetPinName(pPinSource).GetView();

      const ezRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const ezRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        EZ_VERIFY(passToIndex.TryGetValue(pPinTarget->m_pParent, data.m_uiTarget), "Failed to resolve render pass to index");
        data.m_sTargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        connections.PushBack(data);
      }
    }
  }

  ezMemoryStreamContainerWrapperStorage<ezDynamicArray<ezUInt8>> storage(&ref_desc.m_SerializedPipeline);
  ezMemoryStreamWriter memoryWriter(&storage);
  ExportPipeline(passes.GetArrayPtr(), extractors.GetArrayPtr(), connections.GetArrayPtr(), memoryWriter).AssertSuccess("Failed to serialize pipeline");
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
      auto pExtractorType = extractor->GetDynamicRTTI();
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
      EZ_SUCCEED_OR_RETURN(connection.Serialize(stream));
    }
  }

  EZ_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  EZ_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
