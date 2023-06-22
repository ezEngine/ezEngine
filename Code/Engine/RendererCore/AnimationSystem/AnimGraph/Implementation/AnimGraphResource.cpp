#include <RendererCore/RendererCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphResource, 1, ezRTTIDefaultAllocator<ezAnimGraphResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAnimGraphResource);
// clang-format on

ezAnimGraphResource::ezAnimGraphResource()
  : ezResource(ezResource::DoUpdate::OnAnyThread, 0)
{
}

ezAnimGraphResource::~ezAnimGraphResource() = default;

void ezAnimGraphResource::DeserializeAnimGraphState(ezAnimGraph& ref_out)
{
  ref_out.SetInstanceDataAllocator(m_InstanceDataAllocator);

  ezMemoryStreamContainerWrapperStorage<ezDataBuffer> wrapper(&m_Storage);
  ezMemoryStreamReader reader(&wrapper);
  ref_out.Deserialize(reader, m_Nodes).AssertSuccess();
}

ezResourceLoadDesc ezAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  m_Storage.Clear();
  m_Storage.Compact();

  ezResourceLoadDesc d;
  d.m_State = ezResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

ezResourceLoadDesc ezAnimGraphResource::UpdateContent(ezStreamReader* Stream)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  ezUInt32 uiDateSize = 0;
  *Stream >> uiDateSize;
  m_Storage.SetCountUninitialized(uiDateSize);
  Stream->ReadBytes(m_Storage.GetData(), uiDateSize);

  res.m_State = ezResourceState::Loaded;

  AllocNodes().AssertSuccess();

  return res;
}

void ezAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = m_Storage.GetHeapMemoryUsage();
}

ezResult ezAnimGraphResource::AllocNodes()
{
  m_InstanceDataAllocator.ClearDescs();

  ezMemoryStreamContainerWrapperStorage<ezDataBuffer> wrapper(&m_Storage);
  ezMemoryStreamReader reader(&wrapper);

  const auto uiVersion = reader.ReadVersion(6);

  ezUInt32 uiNumNodes = 0;
  reader >> uiNumNodes;
  m_Nodes.SetCount(uiNumNodes);

  ezStringBuilder sTypeName;

  for (auto& node : m_Nodes)
  {
    reader >> sTypeName;
    node = std::move(ezRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<ezAnimGraphNode>());

    EZ_SUCCEED_OR_RETURN(node->DeserializeNode(reader));

    ezInstanceDataDesc desc;
    if (node->GetInstanceDataDesc(desc))
    {
      node->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
    }
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
