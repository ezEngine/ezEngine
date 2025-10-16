#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Containers/IterateBits.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDynamicMeshBufferResource, 1, ezRTTIDefaultAllocator<ezDynamicMeshBufferResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezDynamicMeshBufferResource);
// clang-format on

ezDynamicMeshBufferResource::ezDynamicMeshBufferResource()
  : ezResource(DoUpdate::OnGraphicsResourceThreads, 1)
{
}

ezDynamicMeshBufferResource::~ezDynamicMeshBufferResource()
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    EZ_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
}

ezResourceLoadDesc ezDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(hVertexBuffer);
  }

  ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);

  // we cannot compute this in UpdateMemoryUsage(), so we only read the data there, therefore we need to update this information here
  ModifyMemoryUsage().m_uiMemoryGPU = 0;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezDynamicMeshBufferResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_REPORT_FAILURE("This resource type does not support loading data from file.");

  return ezResourceLoadDesc();
}

void ezDynamicMeshBufferResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  // we cannot compute this data here, so we update it wherever we know the memory usage

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezDynamicMeshBufferResource) + m_PositionData.GetHeapMemoryUsage() + m_NTTData.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage() + m_IndexData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezDynamicMeshBufferResource, ezDynamicMeshBufferResourceDescriptor)
{
  for (auto& hVertexBuffer : m_hVertexBuffers)
  {
    EZ_ASSERT_DEBUG(hVertexBuffer.IsInvalidated(), "Implementation error");
  }
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  ezMeshVertexStreamConfig config;
  {
    config.m_bUseHighPrecision = true;
    config.AddStream(ezMeshVertexStreamType::Position);
    config.AddStream(ezMeshVertexStreamType::NormalTangentAndTexCoord0);
    if (m_Descriptor.m_bColorStream)
    {
      config.AddStream(ezMeshVertexStreamType::Color0);
    }

    EZ_ASSERT_DEBUG(config.GetNormalFormat() == ezGALResourceFormat::RGBAUShortNormalized, "Unexpected normal format");
    EZ_ASSERT_DEBUG(config.GetTangentFormat() == ezGALResourceFormat::RGBAUShortNormalized, "Unexpected tangent format");
    EZ_ASSERT_DEBUG(config.GetTexCoordFormat() == ezGALResourceFormat::XYFloat, "Unexpected texcoord format");
    EZ_ASSERT_DEBUG(config.GetColorFormat() == ezGALResourceFormat::RGBAHalf, "Unexpected color format");

    EZ_ASSERT_DEBUG(config.GetNormalDataOffset() == offsetof(ezDynamicMeshVertexNTT, m_vEncodedNormal), "Unexpected normal offset");
    EZ_ASSERT_DEBUG(config.GetTangentDataOffset() == offsetof(ezDynamicMeshVertexNTT, m_vEncodedTangent), "Unexpected tangent offset");
    EZ_ASSERT_DEBUG(config.GetTexCoord0DataOffset() == offsetof(ezDynamicMeshVertexNTT, m_vTexCoord), "Unexpected texcoord offset");

    config.FillVertexAttributes(m_VertexAttributes);
  }

  const ezUInt32 uiVertexCount = ezMath::Max(1u, m_Descriptor.m_uiMaxVertices);
  const ezUInt32 uiIndexCount = ezGALPrimitiveTopology::GetIndexCount(m_Descriptor.m_Topology, m_Descriptor.m_uiMaxPrimitives);
  const bool bUseIndices = uiIndexCount > 0 && descriptor.m_IndexType != ezGALIndexType::None;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezStringBuilder sName;

  for (ezUInt32 uiIndex : ezIterateBitIndices(config.m_uiTypesMask))
  {
    auto type = static_cast<ezMeshVertexStreamType::Enum>(uiIndex);
    const ezUInt32 uiElementSize = config.GetStreamElementSize(type);

    m_hVertexBuffers[uiIndex] = pDevice->CreateVertexBuffer(uiElementSize, uiVertexCount, ezConstByteArrayPtr(), true);

    sName.SetFormat("{0} Dynamic Vertex Buffer {1}", GetResourceIdOrDescription(), ezMeshVertexStreamType::GetName(type));
    pDevice->GetBuffer(m_hVertexBuffers[uiIndex])->SetDebugName(sName);
  }

  if (bUseIndices)
  {
    m_hIndexBuffer = pDevice->CreateIndexBuffer(descriptor.m_IndexType, uiIndexCount, ezConstByteArrayPtr(), true);

    sName.SetFormat("{0} Dynamic Index Buffer", GetResourceIdOrDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }


  m_PositionData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  m_NTTData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);
  }

  if (bUseIndices)
  {
    m_IndexData.SetCountUninitialized(uiIndexCount * ezGALIndexType::GetSize(descriptor.m_IndexType));
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_PositionData.GetHeapMemoryUsage() + m_NTTData.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage() + m_IndexData.GetHeapMemoryUsage();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezDynamicMeshBufferResource::UploadChangesForNextFrame()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (m_ModifiedPositionDataRange.IsValid())
  {
    auto data = m_PositionData.GetArrayPtr().GetSubArray(m_ModifiedPositionDataRange.m_uiMin, m_ModifiedPositionDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[ezMeshVertexStreamType::Position], data.ToByteArray(), m_ModifiedPositionDataRange.m_uiMin);

    m_ModifiedPositionDataRange.Reset();
  }

  if (m_ModifiedNTTDataRange.IsValid())
  {
    auto data = m_NTTData.GetArrayPtr().GetSubArray(m_ModifiedNTTDataRange.m_uiMin, m_ModifiedNTTDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[ezMeshVertexStreamType::NormalTangentAndTexCoord0], data.ToByteArray(), m_ModifiedNTTDataRange.m_uiMin);

    m_ModifiedNTTDataRange.Reset();
  }

  if (m_ModifiedColorDataRange.IsValid())
  {
    auto data = m_ColorData.GetArrayPtr().GetSubArray(m_ModifiedColorDataRange.m_uiMin, m_ModifiedColorDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[ezMeshVertexStreamType::NormalTangentAndTexCoord0], data.ToByteArray(), m_ModifiedColorDataRange.m_uiMin);

    m_ModifiedColorDataRange.Reset();
  }

  if (m_ModifiedIndexDataRange.IsValid())
  {
    auto data = m_IndexData.GetArrayPtr().GetSubArray(m_ModifiedIndexDataRange.m_uiMin, m_ModifiedIndexDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hIndexBuffer, data, m_ModifiedIndexDataRange.m_uiMin);

    m_ModifiedIndexDataRange.Reset();
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
