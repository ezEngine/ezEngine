#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/RenderContext/RenderContext.h>
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
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");
}

ezResourceLoadDesc ezDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (!m_hVertexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }

  if (!m_hColorBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hColorBuffer);
    m_hColorBuffer.Invalidate();
  }

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

  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(ezDynamicMeshBufferResource) + m_VertexData.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = ModifyMemoryUsage().m_uiMemoryGPU;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezDynamicMeshBufferResource, ezDynamicMeshBufferResourceDescriptor)
{
  EZ_ASSERT_DEBUG(m_hVertexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hIndexBuffer.IsInvalidated(), "Implementation error");
  EZ_ASSERT_DEBUG(m_hColorBuffer.IsInvalidated(), "Implementation error");

  m_Descriptor = descriptor;

  m_VertexData.SetCountUninitialized(m_Descriptor.m_uiMaxVertices);

  {
    ezVertexStreamInfo si;
    si.m_uiOffset = 0;
    si.m_Format = ezGALResourceFormat::XYZFloat;
    si.m_Semantic = ezGALVertexAttributeSemantic::Position;
    si.m_uiElementSize = sizeof(ezVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = ezGALResourceFormat::XYFloat;
    si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
    si.m_uiElementSize = sizeof(ezVec2);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = ezGALResourceFormat::XYZFloat;
    si.m_Semantic = ezGALVertexAttributeSemantic::Normal;
    si.m_uiElementSize = sizeof(ezVec3);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    si.m_uiOffset += si.m_uiElementSize;
    si.m_Format = ezGALResourceFormat::XYZWFloat;
    si.m_Semantic = ezGALVertexAttributeSemantic::Tangent;
    si.m_uiElementSize = sizeof(ezVec4);
    m_VertexDeclaration.m_VertexStreams.PushBack(si);

    if (m_Descriptor.m_bColorStream)
    {
      si.m_uiVertexBufferSlot = 1; // separate buffer
      si.m_uiOffset = 0;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
      si.m_uiElementSize = sizeof(ezColorLinearUB);
      m_VertexDeclaration.m_VertexStreams.PushBack(si);
    }

    m_VertexDeclaration.ComputeHash();
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_hVertexBuffer = pDevice->CreateVertexBuffer(sizeof(ezDynamicMeshVertex), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

  ezStringBuilder sName;
  sName.SetFormat("{0} - Dynamic Vertex Buffer", GetResourceDescription());
  pDevice->GetBuffer(m_hVertexBuffer)->SetDebugName(sName);

  const ezUInt32 uiMaxIndices = ezGALPrimitiveTopology::VerticesPerPrimitive(m_Descriptor.m_Topology) * m_Descriptor.m_uiMaxPrimitives;

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);
    m_hColorBuffer = pDevice->CreateVertexBuffer(sizeof(ezColorLinearUB), m_Descriptor.m_uiMaxVertices /* no initial data -> mutable */);

    sName.SetFormat("{0} - Dynamic Color Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hColorBuffer)->SetDebugName(sName);
  }

  if (m_Descriptor.m_IndexType == ezGALIndexType::UInt)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(ezGALIndexType::UInt, uiMaxIndices /* no initial data -> mutable */);

    sName.SetFormat("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }
  else if (m_Descriptor.m_IndexType == ezGALIndexType::UShort)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    m_hIndexBuffer = pDevice->CreateIndexBuffer(ezGALIndexType::UShort, uiMaxIndices /* no initial data -> mutable */);

    sName.SetFormat("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    pDevice->GetBuffer(m_hIndexBuffer)->SetDebugName(sName);
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezDynamicMeshBufferResource::UpdateGpuBuffer(ezGALCommandEncoder* pGALCommandEncoder, ezUInt32 uiFirstVertex, ezUInt32 uiNumVertices, ezUInt32 uiFirstIndex, ezUInt32 uiNumIndices, ezGALUpdateMode::Enum mode /*= ezGALUpdateMode::Discard*/)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    if (uiNumVertices == ezMath::MaxValue<ezUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    EZ_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(m_hVertexBuffer, sizeof(ezDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    if (uiNumVertices == ezMath::MaxValue<ezUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    EZ_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(m_hColorBuffer, sizeof(ezColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), mode);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && !m_hIndexBuffer.IsInvalidated())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      EZ_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == ezMath::MaxValue<ezUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      EZ_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(ezUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      EZ_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == ezMath::MaxValue<ezUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      EZ_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(m_hIndexBuffer, sizeof(ezUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), mode);
    }
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
