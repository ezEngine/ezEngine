#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
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
    BufferDesc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hGpuBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, m_Transforms.GetArrayPtr().ToByteArray());

    ezGALDevice::GetDefaultDevice()->GetBuffer(m_hGpuBuffer)->SetDebugName("ezSkinningState");
  }
  else
  {
    ezGALDevice::GetDefaultDevice()->UpdateBufferForNextFrame(m_hGpuBuffer, m_Transforms.GetByteArrayPtr());
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
