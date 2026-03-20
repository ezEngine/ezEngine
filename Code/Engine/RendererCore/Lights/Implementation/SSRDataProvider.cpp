#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/SSRDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

void ezSSRData::BindResources(ezRenderContext* pRenderContext)
{
  if (!m_hSSRTexture.IsInvalidated() && ezGALDevice::GetDefaultDevice()->GetTexture(m_hSSRTexture) != nullptr)
  {
    ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
    bindGroup.BindTexture("SSRTexture", m_hSSRTexture);
  }
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSSRDataProvider, 1, ezRTTIDefaultAllocator<ezSSRDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSSRDataProvider::ezSSRDataProvider() = default;

ezSSRDataProvider::~ezSSRDataProvider() = default;

void ezSSRDataProvider::SetSSRTexture(ezGALTextureHandle hTexture)
{
  m_Data.m_hSSRTexture = hTexture;
}

void* ezSSRDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  return &m_Data;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SSRDataProvider);