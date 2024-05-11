#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Shader/Types.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkinnedMeshRenderData, 1, ezRTTIDefaultAllocator<ezSkinnedMeshRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezSkinnedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hSkinningTransforms.GetInternalID().m_Data);
}

ezSkinningState::ezSkinningState() = default;

ezSkinningState::~ezSkinningState()
{
  Clear();
}

void ezSkinningState::Clear()
{
  if (!m_hGpuBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGpuBuffer);
    m_hGpuBuffer.Invalidate();
  }

  m_bTransformsUpdated[0] = nullptr;
  m_bTransformsUpdated[1] = nullptr;
  m_Transforms.Clear();
}

void ezSkinningState::TransformsChanged()
{
  if (m_hGpuBuffer.IsInvalidated())
  {
    if (m_Transforms.GetCount() == 0)
      return;

    ezGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(ezShaderTransform);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_Transforms.GetCount();
    BufferDesc.m_BufferFlags = ezGALBufferFlags::StructuredBuffer | ezGALBufferFlags::ShaderResource;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hGpuBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, m_Transforms.GetArrayPtr().ToByteArray());

    m_bTransformsUpdated[0] = std::make_shared<bool>(true);
    m_bTransformsUpdated[1] = std::make_shared<bool>(true);
  }
  else
  {
    const ezUInt32 uiRenIdx = ezRenderWorld::GetDataIndexForExtraction();
    *m_bTransformsUpdated[uiRenIdx] = false;
  }
}

void ezSkinningState::FillSkinnedMeshRenderData(ezSkinnedMeshRenderData& ref_renderData) const
{
  ref_renderData.m_hSkinningTransforms = m_hGpuBuffer;

  const ezUInt32 uiExIdx = ezRenderWorld::GetDataIndexForExtraction();

  if (m_bTransformsUpdated[uiExIdx] && *m_bTransformsUpdated[uiExIdx] == false)
  {
    auto pSkinningMatrices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezShaderTransform, m_Transforms.GetCount());
    pSkinningMatrices.CopyFrom(m_Transforms);

    ref_renderData.m_pNewSkinningTransformData = pSkinningMatrices.ToByteArray();
    ref_renderData.m_bTransformsUpdated = m_bTransformsUpdated[uiExIdx];
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
