#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <Shaders/Common/ObjectConstants.h>

ezInstanceData::ezInstanceData(ezUInt32 uiMaxInstanceCount, bool bTransient)
{
  CreateBuffer(uiMaxInstanceCount, bTransient);

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezObjectConstants>();
}

ezInstanceData::~ezInstanceData()
{
  m_InstanceDataBuffer.Deinitialize();
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezInstanceData::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezBindGroupBuilder& bindGroupDraw = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
  bindGroupDraw.BindBuffer("perInstanceData", m_InstanceDataBuffer.GetCurrentBuffer());

  bindGroupDraw.BindBuffer("ezObjectConstants", m_hConstantBuffer);
}

ezArrayPtr<ezPerInstanceData> ezInstanceData::GetInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount, ezUInt32& out_uiOffset)
{
  uiCount = ezMath::Min(uiCount, m_uiBufferSize);
  if (m_uiBufferOffset + uiCount > m_uiBufferSize)
  {
    m_uiBufferOffset = 0;
  }

  if (m_uiBufferOffset == 0)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALBufferHandle hBuffer = m_InstanceDataBuffer.GetNewBuffer();
    ezBindGroupBuilder& bindGroupDraw = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
    bindGroupDraw.BindBuffer("perInstanceData", hBuffer);
  }


  out_uiOffset = m_uiBufferOffset;
  return m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
}

void ezInstanceData::UpdateInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount)
{

  EZ_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezUInt32 uiDestOffset = m_uiBufferOffset * sizeof(ezPerInstanceData);
  auto pSourceData = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  ezGALUpdateMode::Enum updateMode = ezGALUpdateMode::AheadOfTime;

  pGALCommandEncoder->UpdateBuffer(m_InstanceDataBuffer.GetCurrentBuffer(), uiDestOffset, pSourceData.ToByteArray(), updateMode);

  ezObjectConstants* pConstants = pRenderContext->GetConstantBufferData<ezObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void ezInstanceData::CreateBuffer(ezUInt32 uiSize, bool bTransient)
{
  m_uiBufferSize = uiSize;
  m_PerInstanceData.SetCountUninitialized(m_uiBufferSize);

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerInstanceData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiSize;
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
  desc.m_ResourceAccess.m_bImmutable = false;
  if (bTransient)
    desc.m_BufferFlags |= ezGALBufferUsageFlags::Transient;

  m_InstanceDataBuffer.Initialize(desc, "InstanceDataProvider - StructuredBuffer");
}

void ezInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInstanceDataProvider, 1, ezRTTIDefaultAllocator<ezInstanceDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezInstanceDataProvider::ezInstanceDataProvider() = default;
ezInstanceDataProvider::~ezInstanceDataProvider() = default;

void* ezInstanceDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_InstanceDataProvider);
