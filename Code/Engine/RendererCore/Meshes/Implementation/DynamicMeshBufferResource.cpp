#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/IterateBits.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>

namespace
{
  static ezMutex s_ResourcesToUploadMutex;
  static ezHashSet<ezDynamicMeshBufferResource*> s_ResourcesToUpload;
} // namespace

struct ezDynamicMeshBufferManager
{
  static void AddResourceToUpload(ezDynamicMeshBufferResource* pResource)
  {
    EZ_LOCK(s_ResourcesToUploadMutex);
    s_ResourcesToUpload.Insert(pResource);
  }

  static void RemoveResourceToUpload(ezDynamicMeshBufferResource* pResource)
  {
    EZ_LOCK(s_ResourcesToUploadMutex);
    s_ResourcesToUpload.Remove(pResource);
  }

  static void OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
  {
    if (e.m_Type != ezRenderWorldExtractionEvent::Type::EndExtraction)
      return;

    EZ_LOCK(s_ResourcesToUploadMutex);

    for (auto it : s_ResourcesToUpload)
    {
      it->UploadChangesForNextFrame();
    }

    s_ResourcesToUpload.Clear();
  }
};

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, DynamicMeshBufferManager)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezRenderWorld::GetExtractionEvent().AddEventHandler(ezDynamicMeshBufferManager::OnExtractionEvent);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezRenderWorld::GetExtractionEvent().RemoveEventHandler(ezDynamicMeshBufferManager::OnExtractionEvent);
  }
EZ_END_SUBSYSTEM_DECLARATION;

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
  ezDynamicMeshBufferManager::RemoveResourceToUpload(this);

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

void ezDynamicMeshBufferResource::MarkAsDirty()
{
  ezDynamicMeshBufferManager::AddResourceToUpload(this);
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

    pDevice->UpdateBufferForNextFrame(m_hVertexBuffers[ezMeshVertexStreamType::Color0], data.ToByteArray(), m_ModifiedColorDataRange.m_uiMin);

    m_ModifiedColorDataRange.Reset();
  }

  if (m_ModifiedIndexDataRange.IsValid())
  {
    auto data = m_IndexData.GetArrayPtr().GetSubArray(m_ModifiedIndexDataRange.m_uiMin, m_ModifiedIndexDataRange.GetCount());

    pDevice->UpdateBufferForNextFrame(m_hIndexBuffer, data, m_ModifiedIndexDataRange.m_uiMin);

    m_ModifiedIndexDataRange.Reset();
  }
}

// static
void ezDynamicMeshBufferResource::CreateGridXY(ezDynamicMeshBufferResource* pDynamicMeshBuffer, const ezVec2& vSize, const ezVec2U32& vNumVertices, const ezVec2& vTextureScale)
{
  EZ_ASSERT_DEV(vNumVertices.x > 1 && vNumVertices.y > 1, "Invalid number of vertices");

  const ezUInt32 uiNumVertsX = vNumVertices.x;
  const ezUInt32 uiNumVertsY = vNumVertices.y;
  const ezUInt32 uiNumSegmentsX = uiNumVertsX - 1;
  const ezUInt32 uiNumSegmentsY = uiNumVertsY - 1;

  EZ_ASSERT_DEV(pDynamicMeshBuffer->m_Descriptor.m_uiMaxVertices == uiNumVertsX * uiNumVertsY, "Invalid number of vertices");
  EZ_ASSERT_DEV(pDynamicMeshBuffer->m_Descriptor.m_uiMaxPrimitives = uiNumSegmentsX * uiNumSegmentsY * 2, "Invalid number of primitives");

  {
    const ezVec3 dirX = ezVec3(1, 0, 0);
    const ezVec3 dirY = ezVec3(0, 1, 0);

    ezDynamicMeshVertexNTT v;
    v.EncodeNormal(ezVec3(0, 0, 1));
    v.EncodeTangent(dirX, 1.0f);

    ezVec2 dist = vSize;
    dist.x /= (float)uiNumSegmentsX;
    dist.y /= (float)uiNumSegmentsY;

    const float fDivU = (1.0f / uiNumSegmentsX) * vTextureScale.x;
    const float fDivV = (1.0f / uiNumSegmentsY) * vTextureScale.y;

    auto positions = pDynamicMeshBuffer->AccessPositionData();
    auto ntt = pDynamicMeshBuffer->AccessNormalTangentTexCoord0Data();

    for (ezUInt32 y = 0; y < uiNumVertsY; ++y)
    {
      for (ezUInt32 x = 0; x < uiNumVertsX; ++x)
      {
        const ezUInt32 idx = (y * uiNumVertsX) + x;

        positions[idx] = dirX * (x * dist.x) + dirY * (y * dist.y);
        ntt[idx].m_vEncodedNormal = v.m_vEncodedNormal;
        ntt[idx].m_vEncodedTangent = v.m_vEncodedTangent;
        ntt[idx].m_vTexCoord = ezVec2(x * fDivU, y * fDivV);
      }
    }
  }

  {
    auto indices = pDynamicMeshBuffer->AccessIndex16Data();

    ezUInt32 tidx = 0;
    ezUInt32 vidx = 0;
    for (ezUInt32 y = 0; y < uiNumSegmentsY; ++y)
    {
      for (ezUInt32 x = 0; x < uiNumSegmentsX; ++x, ++vidx)
      {
        indices[tidx++] = vidx;
        indices[tidx++] = vidx + 1;
        indices[tidx++] = vidx + uiNumVertsX;

        indices[tidx++] = vidx + 1;
        indices[tidx++] = vidx + uiNumVertsX + 1;
        indices[tidx++] = vidx + uiNumVertsX;
      }

      ++vidx;
    }
  }
}

void ezDynamicMeshBufferResource::CalculateGridNormalAndTangents(ezDynamicMeshBufferResource* pDynamicMeshBuffer, const ezVec2U32& vNumVertices)
{
  auto positions = pDynamicMeshBuffer->AccessPositionData();
  auto ntt = pDynamicMeshBuffer->AccessNormalTangentTexCoord0Data();

  const ezUInt32 width = vNumVertices.x;
  const ezUInt32 height = vNumVertices.y;
  const ezUInt32 widthM1 = width - 1;
  const ezUInt32 heightM1 = height - 1;

  ezUInt32 topIdx = 0;

  ezUInt32 vidx = 0;
  for (ezUInt32 y = 0; y < height; ++y)
  {
    ezUInt32 leftIdx = 0;
    const ezUInt32 bottomIdx = ezMath::Min<ezUInt32>(y + 1, heightM1);

    const ezUInt32 yOff = y * width;
    const ezUInt32 yOffTop = topIdx * width;
    const ezUInt32 yOffBottom = bottomIdx * width;

    for (ezUInt32 x = 0; x < width; ++x, ++vidx)
    {
      const ezUInt32 rightIdx = ezMath::Min<ezUInt32>(x + 1, widthM1);

      const ezSimdVec4f leftPos = ezSimdConversion::ToVec3(positions[yOff + leftIdx]);
      const ezSimdVec4f rightPos = ezSimdConversion::ToVec3(positions[yOff + rightIdx]);
      const ezSimdVec4f topPos = ezSimdConversion::ToVec3(positions[yOffTop + x]);
      const ezSimdVec4f bottomPos = ezSimdConversion::ToVec3(positions[yOffBottom + x]);

      const ezSimdVec4f leftToRight = rightPos - leftPos;
      const ezSimdVec4f bottomToTop = topPos - bottomPos;
      ezSimdVec4f normal = -leftToRight.CrossRH(bottomToTop);
      normal.NormalizeIfNotZero<3>(ezSimdVec4f(0, 0, 1));

      ezSimdVec4f tangent = leftToRight;
      tangent.NormalizeIfNotZero<3>(ezSimdVec4f(1, 0, 0));

      ntt[vidx].EncodeNormal(ezSimdConversion::ToVec3(normal));
      ntt[vidx].EncodeTangent(ezSimdConversion::ToVec3(tangent), 1.0f);

      leftIdx = x;
    }

    topIdx = y;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_DynamicMeshBufferResource);
