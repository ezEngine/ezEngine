#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Serialization/BinarySerializer.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct RenderPipelineResourceLoaderConnectionInternal
{
  ezUuid m_Source;
  ezUuid m_Target;
  ezString m_SourcePin;
  ezString m_TargetPin;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, RenderPipelineResourceLoaderConnectionInternal);

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(RenderPipelineResourceLoaderConnectionInternal, ezNoBase, 1, ezRTTIDefaultAllocator<RenderPipelineResourceLoaderConnectionInternal>)
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

void ezRenderPipelineRttiConverterContext::Clear()
{
  ezRttiConverterContext::Clear();

  m_pRenderPipeline = nullptr;
}

ezInternal::NewInstance<void> ezRenderPipelineRttiConverterContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti->IsDerivedFrom<ezRenderPipelinePass>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      ezLog::Error("Failed to create ezRenderPipelinePass because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto pass = pRtti->GetAllocator()->Allocate<ezRenderPipelinePass>();
    m_pRenderPipeline->AddPass(pass);

    RegisterObject(guid, pRtti, pass);
    return pass;
  }
  else if (pRtti->IsDerivedFrom<ezExtractor>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      ezLog::Error("Failed to create ezExtractor because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    auto extractor = pRtti->GetAllocator()->Allocate<ezExtractor>();
    m_pRenderPipeline->AddExtractor(extractor);

    RegisterObject(guid, pRtti, extractor);
    return extractor;
  }
  else
  {
    return ezRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void ezRenderPipelineRttiConverterContext::DeleteObject(const ezUuid& guid)
{
  ezRttiConverterObject object = GetObjectByGUID(guid);
  const ezRTTI* pRtti = object.m_pType;
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");
  if (pRtti->IsDerivedFrom<ezRenderPipelinePass>())
  {
    ezRenderPipelinePass* pPass = static_cast<ezRenderPipelinePass*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemovePass(pPass);
  }
  else if (pRtti->IsDerivedFrom<ezExtractor>())
  {
    ezExtractor* pExtractor = static_cast<ezExtractor*>(object.m_pObject);

    UnregisterObject(guid);
    m_pRenderPipeline->RemoveExtractor(pExtractor);
  }
  else
  {
    ezRttiConverterContext::DeleteObject(guid);
  }
}

// static
ezInternal::NewInstance<ezRenderPipeline> ezRenderPipelineResourceLoader::CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  ezRenderPipelineRttiConverterContext context;
  context.m_pRenderPipeline = pPipeline;

  ezRawMemoryStreamReader memoryReader(desc.m_SerializedPipeline);

  ezAbstractObjectGraph graph;
  ezAbstractGraphBinarySerializer::Read(memoryReader, &graph);

  ezRttiConverterReader rttiConverter(&graph, &context);

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto pNode = it.Value();
    ezRTTI* pType = ezRTTI::FindTypeByName(pNode->GetType());
    if (pType && pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      auto pPass = rttiConverter.CreateObjectFromNode(pNode);
      if (!pPass)
      {
        ezLog::Error("Failed to deserialize ezRenderPipelinePass!");
      }
    }
    else if (pType && pType->IsDerivedFrom<ezExtractor>())
    {
      auto pExtractor = rttiConverter.CreateObjectFromNode(pNode);
      if (!pExtractor)
      {
        ezLog::Error("Failed to deserialize ezExtractor!");
      }
    }
  }

  auto pType = ezGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  ezStringBuilder tmp;

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    if (pNode->GetNodeName() != "Connection")
      continue;

    RenderPipelineResourceLoaderConnectionInternal data;
    rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

    auto objectSource = context.GetObjectByGUID(data.m_Source);
    if (objectSource.m_pObject == nullptr || !objectSource.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      ezLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", ezConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    auto objectTarget = context.GetObjectByGUID(data.m_Target);
    if (objectTarget.m_pObject == nullptr || !objectTarget.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      ezLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", ezConversionUtils::ToString(guid, tmp), data.m_TargetPin);
      continue;
    }

    ezRenderPipelinePass* pSource = static_cast<ezRenderPipelinePass*>(objectSource.m_pObject);
    ezRenderPipelinePass* pTarget = static_cast<ezRenderPipelinePass*>(objectTarget.m_pObject);

    if (!pPipeline->Connect(pSource, data.m_SourcePin, pTarget, data.m_TargetPin))
    {
      ezLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), data.m_SourcePin, pTarget->GetName(), data.m_TargetPin);
    }
  }

  return pPipeline;
}

// static
void ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& ref_desc)
{
  ezRenderPipelineRttiConverterContext context;

  ezAbstractObjectGraph graph;

  ezRttiConverterWriter rttiConverter(&graph, &context, false, true);

  ezHybridArray<const ezRenderPipelinePass*, 16> passes;
  pPipeline->GetPasses(passes);

  // Need to serialize all passes first so we have guids for each to be referenced in the connections.
  for (auto pPass : passes)
  {
    ezUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pPass->GetDynamicRTTI(), const_cast<ezRenderPipelinePass*>(pPass));
    rttiConverter.AddObjectToGraph(const_cast<ezRenderPipelinePass*>(pPass));
  }
  ezHybridArray<const ezExtractor*, 16> extractors;
  pPipeline->GetExtractors(extractors);
  for (auto pExtractor : extractors)
  {
    ezUuid guid;
    guid.CreateNewUuid();
    context.RegisterObject(guid, pExtractor->GetDynamicRTTI(), const_cast<ezExtractor*>(pExtractor));
    rttiConverter.AddObjectToGraph(const_cast<ezExtractor*>(pExtractor));
  }

  auto pType = ezGetStaticRTTI<RenderPipelineResourceLoaderConnectionInternal>();
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    auto objectSoure = context.GetObjectByGUID(pNode->GetGuid());

    if (objectSoure.m_pObject == nullptr || !objectSoure.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      continue;
    }
    ezRenderPipelinePass* pSource = static_cast<ezRenderPipelinePass*>(objectSoure.m_pObject);

    RenderPipelineResourceLoaderConnectionInternal data;
    data.m_Source = pNode->GetGuid();

    auto outputs = pSource->GetOutputPins();
    for (const ezRenderPipelineNodePin* pPinSource : outputs)
    {
      data.m_SourcePin = pSource->GetPinName(pPinSource).GetView();

      const ezRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const ezRenderPipelineNodePin* pPinTarget : pConnection->m_Inputs)
      {
        data.m_Target = context.GetObjectGUID(pPinTarget->m_pParent->GetDynamicRTTI(), pPinTarget->m_pParent);
        data.m_TargetPin = pPinTarget->m_pParent->GetPinName(pPinTarget).GetView();

        ezUuid connectionGuid;
        connectionGuid.CreateNewUuid();
        context.RegisterObject(connectionGuid, pType, &data);
        rttiConverter.AddObjectToGraph(pType, &data, "Connection");
      }
    }
  }

  ezMemoryStreamContainerWrapperStorage<ezDynamicArray<ezUInt8>> storage(&ref_desc.m_SerializedPipeline);

  ezMemoryStreamWriter memoryWriter(&storage);
  ezAbstractGraphBinarySerializer::Write(memoryWriter, &graph);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
