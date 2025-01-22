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
  EZ_ASSERT_DEBUG(!m_VertexBuffer.IsInitialized(), "Implementation error");
  EZ_ASSERT_DEBUG(!m_IndexBuffer.IsInitialized(), "Implementation error");
  EZ_ASSERT_DEBUG(!m_ColorBuffer.IsInitialized(), "Implementation error");
}

ezResourceLoadDesc ezDynamicMeshBufferResource::UnloadData(Unload WhatToUnload)
{
  if (m_VertexBuffer.IsInitialized())
  {
    m_VertexBuffer.Deinitialize();
  }

  if (m_IndexBuffer.IsInitialized())
  {
    m_IndexBuffer.Deinitialize();
  }

  if (m_ColorBuffer.IsInitialized())
  {
    m_ColorBuffer.Deinitialize();
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
  EZ_ASSERT_DEBUG(!m_VertexBuffer.IsInitialized(), "Implementation error");
  EZ_ASSERT_DEBUG(!m_IndexBuffer.IsInitialized(), "Implementation error");
  EZ_ASSERT_DEBUG(!m_ColorBuffer.IsInitialized(), "Implementation error");

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
  ezStringBuilder sName;
  {
    ezGALBufferCreationDescription vertexDesc;
    vertexDesc.m_uiStructSize = sizeof(ezDynamicMeshVertex);
    vertexDesc.m_uiTotalSize = sizeof(ezDynamicMeshVertex) * ezMath::Max(1u, m_Descriptor.m_uiMaxVertices);
    vertexDesc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
    vertexDesc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Vertex Buffer", GetResourceDescription());
    m_VertexBuffer.Initialize(vertexDesc, sName);
    m_VertexBuffer.GetNewBuffer();
  }

  const ezUInt32 uiMaxIndices = ezGALPrimitiveTopology::GetIndexCount(m_Descriptor.m_Topology, m_Descriptor.m_uiMaxPrimitives);

  if (m_Descriptor.m_bColorStream)
  {
    m_ColorData.SetCountUninitialized(uiMaxIndices);

    ezGALBufferCreationDescription vertexDesc;
    vertexDesc.m_uiStructSize = sizeof(ezColorLinearUB);
    vertexDesc.m_uiTotalSize = sizeof(ezColorLinearUB) * ezMath::Max(1u, m_Descriptor.m_uiMaxVertices);
    vertexDesc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
    vertexDesc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Color Buffer", GetResourceDescription());
    m_ColorBuffer.Initialize(vertexDesc, sName);
    m_ColorBuffer.GetNewBuffer();
  }

  if (m_Descriptor.m_IndexType == ezGALIndexType::UInt)
  {
    m_Index32Data.SetCountUninitialized(uiMaxIndices);

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = ezGALIndexType::GetSize(ezGALIndexType::UInt);
    desc.m_uiTotalSize = desc.m_uiStructSize * ezMath::Max(1u, uiMaxIndices);
    desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Index32 Buffer", GetResourceDescription());
    m_IndexBuffer.Initialize(desc, sName);
    m_IndexBuffer.GetNewBuffer();
  }
  else if (m_Descriptor.m_IndexType == ezGALIndexType::UShort)
  {
    m_Index16Data.SetCountUninitialized(uiMaxIndices);

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = ezGALIndexType::GetSize(ezGALIndexType::UShort);
    desc.m_uiTotalSize = desc.m_uiStructSize * ezMath::Max(1u, uiMaxIndices);
    desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    sName.SetFormat("{0} - Dynamic Index16 Buffer", GetResourceDescription());
    m_IndexBuffer.Initialize(desc, sName);
    m_IndexBuffer.GetNewBuffer();
  }

  // we only know the memory usage here, so we write it back to the internal variable directly and then read it in UpdateMemoryUsage() again
  ModifyMemoryUsage().m_uiMemoryGPU = m_VertexData.GetHeapMemoryUsage() + m_Index32Data.GetHeapMemoryUsage() + m_Index16Data.GetHeapMemoryUsage() + m_ColorData.GetHeapMemoryUsage();

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezDynamicMeshBufferResource::UpdateGpuBuffer(ezGALCommandEncoder* pGALCommandEncoder, ezUInt32 uiFirstVertex, ezUInt32 uiNumVertices, ezUInt32 uiFirstIndex, ezUInt32 uiNumIndices, bool bCreateNewBuffer)
{
  if (m_bAccessedVB && uiNumVertices > 0)
  {
    ezGALBufferHandle hVertexBuffer = m_VertexBuffer.GetCurrentBuffer();
    if (bCreateNewBuffer)
      hVertexBuffer = m_VertexBuffer.GetNewBuffer();

    if (uiNumVertices == ezMath::MaxValue<ezUInt32>())
      uiNumVertices = m_VertexData.GetCount() - uiFirstVertex;

    EZ_ASSERT_DEV(uiNumVertices <= m_VertexData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_VertexData.GetCount());

    m_bAccessedVB = false;

    pGALCommandEncoder->UpdateBuffer(hVertexBuffer, sizeof(ezDynamicMeshVertex) * uiFirstVertex, m_VertexData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), ezGALUpdateMode::AheadOfTime);
  }

  if (m_bAccessedCB && uiNumVertices > 0)
  {
    ezGALBufferHandle hColorBuffer = m_ColorBuffer.GetCurrentBuffer();
    if (bCreateNewBuffer)
      hColorBuffer = m_ColorBuffer.GetNewBuffer();

    if (uiNumVertices == ezMath::MaxValue<ezUInt32>())
      uiNumVertices = m_ColorData.GetCount() - uiFirstVertex;

    EZ_ASSERT_DEV(uiNumVertices <= m_ColorData.GetCount(), "Can't upload {} vertices, the buffer was allocated to hold a maximum of {} vertices.", uiNumVertices, m_ColorData.GetCount());

    m_bAccessedCB = false;

    pGALCommandEncoder->UpdateBuffer(hColorBuffer, sizeof(ezColorLinearUB) * uiFirstVertex, m_ColorData.GetArrayPtr().GetSubArray(uiFirstVertex, uiNumVertices).ToByteArray(), ezGALUpdateMode::AheadOfTime);
  }

  if (m_bAccessedIB && uiNumIndices > 0 && m_IndexBuffer.IsInitialized())
  {
    m_bAccessedIB = false;

    if (!m_Index16Data.IsEmpty())
    {
      ezGALBufferHandle hIndexBuffer = m_IndexBuffer.GetCurrentBuffer();
      if (bCreateNewBuffer)
        hIndexBuffer = m_IndexBuffer.GetNewBuffer();

      EZ_ASSERT_DEV(uiFirstIndex < m_Index16Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == ezMath::MaxValue<ezUInt32>())
        uiNumIndices = m_Index16Data.GetCount() - uiFirstIndex;

      EZ_ASSERT_DEV(uiNumIndices <= m_Index16Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index16Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(hIndexBuffer, sizeof(ezUInt16) * uiFirstIndex, m_Index16Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), ezGALUpdateMode::AheadOfTime);
    }
    else if (!m_Index32Data.IsEmpty())
    {
      ezGALBufferHandle hIndexBuffer = m_IndexBuffer.GetCurrentBuffer();
      if (bCreateNewBuffer)
        hIndexBuffer = m_IndexBuffer.GetNewBuffer();

      EZ_ASSERT_DEV(uiFirstIndex < m_Index32Data.GetCount(), "Invalid first index value {}", uiFirstIndex);

      if (uiNumIndices == ezMath::MaxValue<ezUInt32>())
        uiNumIndices = m_Index32Data.GetCount() - uiFirstIndex;

      EZ_ASSERT_DEV(uiNumIndices <= m_Index32Data.GetCount(), "Can't upload {} indices, the buffer was allocated to hold a maximum of {} indices.", uiNumIndices, m_Index32Data.GetCount());

      pGALCommandEncoder->UpdateBuffer(hIndexBuffer, sizeof(ezUInt32) * uiFirstIndex, m_Index32Data.GetArrayPtr().GetSubArray(uiFirstIndex, uiNumIndices).ToByteArray(), ezGALUpdateMode::AheadOfTime);
    }
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
