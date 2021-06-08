#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

ezInstanceData::ezInstanceData(ezUInt32 uiMaxInstanceCount /*= 1024*/)
  : m_uiBufferSize(0)
  , m_uiBufferOffset(0)
{
  CreateBuffer(uiMaxInstanceCount);

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezObjectConstants>();
}

ezInstanceData::~ezInstanceData()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hInstanceDataBuffer);

  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezInstanceData::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pRenderContext->BindBuffer("perInstanceData", pDevice->GetDefaultResourceView(m_hInstanceDataBuffer));
  pRenderContext->BindConstantBuffer("ezObjectConstants", m_hConstantBuffer);
}

ezArrayPtr<ezPerInstanceData> ezInstanceData::GetInstanceData(ezUInt32 uiCount, ezUInt32& out_uiOffset)
{
  uiCount = ezMath::Min(uiCount, m_uiBufferSize);
  if (m_uiBufferOffset + uiCount > m_uiBufferSize)
  {
    m_uiBufferOffset = 0;
  }

  out_uiOffset = m_uiBufferOffset;
  return m_perInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
}

void ezInstanceData::UpdateInstanceData(ezRenderContext* pRenderContext, ezUInt32 uiCount)
{
  EZ_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  ezGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  ezUInt32 uiDestOffset = m_uiBufferOffset * sizeof(ezPerInstanceData);
  auto pSourceData = m_perInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  ezGALUpdateMode::Enum updateMode = (m_uiBufferOffset == 0) ? ezGALUpdateMode::Discard : ezGALUpdateMode::NoOverwrite;

  pGALCommandEncoder->UpdateBuffer(m_hInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray(), updateMode);


  ezObjectConstants* pConstants = pRenderContext->GetConstantBufferData<ezObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void ezInstanceData::CreateBuffer(ezUInt32 uiSize)
{
  m_uiBufferSize = uiSize;
  m_perInstanceData.SetCountUninitialized(m_uiBufferSize);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerInstanceData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiSize;
  desc.m_BufferType = ezGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hInstanceDataBuffer = pDevice->CreateBuffer(desc);
}

void ezInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInstanceDataProvider, 1, ezRTTIDefaultAllocator<ezInstanceDataProvider>)
  {
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezInstanceDataProvider::ezInstanceDataProvider() {}

ezInstanceDataProvider::~ezInstanceDataProvider() {}

void* ezInstanceDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_InstanceDataProvider);
