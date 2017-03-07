#include <PCH.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <Foundation/Serialization/BinarySerializer.h>

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct ConnectionInternal
{
  ConnectionInternal() {}
  ConnectionInternal(const char* szPinSource, const ezUuid& target, const char* szPinTarget)
    : m_SourcePin(szPinSource), m_Target(target), m_TargetPin(szPinTarget) {}

  ezString m_SourcePin;
  ezUuid m_Target;
  ezString m_TargetPin;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ConnectionInternal);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ConnectionInternal, ezNoBase, 1, ezRTTIDefaultAllocator<ConnectionInternal>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SourcePin", m_SourcePin),
    EZ_MEMBER_PROPERTY("Target", m_Target),
    EZ_MEMBER_PROPERTY("TargetPin", m_TargetPin),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

struct NodeDataInternal
{
  ezVec2 m_NodePos;
  ezDynamicArray<ConnectionInternal> m_Connections;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, NodeDataInternal);

EZ_BEGIN_STATIC_REFLECTED_TYPE(NodeDataInternal, ezNoBase, 1, ezRTTIDefaultAllocator<NodeDataInternal>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Node::Pos", m_NodePos),
    EZ_ARRAY_MEMBER_PROPERTY("Node::Connections", m_Connections),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE


void ezRenderPipelineRttiConverterContext::Clear()
{
  ezRttiConverterContext::Clear();

  m_pRenderPipeline = nullptr;
}

void* ezRenderPipelineRttiConverterContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti->IsDerivedFrom<ezRenderPipelinePass>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      ezLog::Error("Failed to create ezRenderPipelinePass because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    // TODO: Refactor rtti allocator to allow for different allocators and the same return value as EZ_DEFAULT_NEW.
    ezUniquePtr<ezRenderPipelinePass> pass(static_cast<ezRenderPipelinePass*>(pRtti->GetAllocator()->Allocate()), ezFoundation::GetDefaultAllocator());
    ezRenderPipelinePass* pPass = pass.Borrow();
    m_pRenderPipeline->AddPass(std::move(pass));

    RegisterObject(guid, pRtti, pPass);
    return pPass;
  }
  else if (pRtti->IsDerivedFrom<ezExtractor>())
  {
    if (!pRtti->GetAllocator()->CanAllocate())
    {
      ezLog::Error("Failed to create ezExtractor because '{0}' cannot allocate!", pRtti->GetTypeName());
      return nullptr;
    }

    // TODO: Refactor rtti allocator to allow for different allocators and the same return value as EZ_DEFAULT_NEW.
    ezUniquePtr<ezExtractor> extractor(static_cast<ezExtractor*>(pRtti->GetAllocator()->Allocate()), ezFoundation::GetDefaultAllocator());
    ezExtractor* pExtractor = extractor.Borrow();
    m_pRenderPipeline->AddExtractor(std::move(extractor));

    RegisterObject(guid, pRtti, pExtractor);
    return pExtractor;
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

//static
ezInternal::NewInstance<ezRenderPipeline> ezRenderPipelineResourceLoader::CreateRenderPipeline(const ezRenderPipelineResourceDescriptor& desc)
{
  auto pPipeline = EZ_DEFAULT_NEW(ezRenderPipeline);
  ezRenderPipelineRttiConverterContext context;
  context.m_pRenderPipeline = pPipeline;

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);
  memoryWriter.WriteBytes(desc.m_SerializedPipeline.GetData(), desc.m_SerializedPipeline.GetCount());

  ezMemoryStreamReader memoryReader(&streamStorage);

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

  auto pType = ezGetStaticRTTI<NodeDataInternal>();
  NodeDataInternal data;

  ezStringBuilder tmp;

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    auto objectSoure = context.GetObjectByGUID(guid);

    if (!objectSoure.m_pObject || !objectSoure.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      continue;
    }
    ezRenderPipelinePass* pSource = static_cast<ezRenderPipelinePass*>(objectSoure.m_pObject);

    data.m_Connections.Clear();
    rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

    for (const auto& con : data.m_Connections)
    {
      auto objectTarget = context.GetObjectByGUID(con.m_Target);
      if (!objectTarget.m_pObject || !objectTarget.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
      {
        ezLog::Error("Failed to retrieve connection target '{0}' with pin '{1}'", ezConversionUtils::ToString(guid, tmp), con.m_TargetPin);
        continue;
      }
      ezRenderPipelinePass* pTarget = static_cast<ezRenderPipelinePass*>(objectTarget.m_pObject);

      if (!pPipeline->Connect(pSource, con.m_SourcePin, pTarget, con.m_TargetPin))
      {
        ezLog::Error("Failed to connect '{0}'::'{1}' to '{2}'::'{3}'!", pSource->GetName(), con.m_SourcePin, pTarget->GetName(), con.m_TargetPin);
      }
    }
  }

  return pPipeline;
}

//static
void ezRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(const ezRenderPipeline* pPipeline, ezRenderPipelineResourceDescriptor& desc)
{
  ezRenderPipelineRttiConverterContext context;

  ezMemoryStreamStorage streamStorage;
  ezMemoryStreamWriter memoryWriter(&streamStorage);

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


  auto pType = ezGetStaticRTTI<NodeDataInternal>();
  NodeDataInternal data;
  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    auto objectSoure = context.GetObjectByGUID(guid);

    if (!objectSoure.m_pObject || !objectSoure.m_pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      continue;
    }
    ezRenderPipelinePass* pSource = static_cast<ezRenderPipelinePass*>(objectSoure.m_pObject);

    data.m_Connections.Clear();

    auto outputs = pSource->GetOutputPins();
    for (const ezNodePin* pPinSource : outputs)
    {
      const ezRenderPipelinePassConnection* pConnection = pPipeline->GetOutputConnection(pSource, pSource->GetPinName(pPinSource));
      if (!pConnection)
        continue;

      for (const ezNodePin* pPinTarget : pConnection->m_Inputs)
      {
        const ezUuid targetGuid = context.GetObjectGUID(pPinTarget->m_pParent->GetDynamicRTTI(), pPinTarget->m_pParent);
        EZ_ASSERT_DEBUG(targetGuid.IsValid(), "Connection target was not serialized in previous step!");
        data.m_Connections.PushBack(ConnectionInternal(pSource->GetPinName(pPinSource).GetData(), targetGuid, pPinTarget->m_pParent->GetPinName(pPinTarget).GetData()));
      }
    }

    rttiConverter.AddProperties(pNode, pType, &data);

  }

  ezAbstractGraphBinarySerializer::Write(memoryWriter, &graph);
  desc.m_SerializedPipeline.SetCount(streamStorage.GetStorageSize());
  ezMemoryUtils::Copy(desc.m_SerializedPipeline.GetData(), streamStorage.GetData(), streamStorage.GetStorageSize());
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelineResourceLoader);
