#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ScreenSpaceShadowDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>

void ezScreenSpaceShadowData::BindResources(ezRenderContext* pRenderContext)
{
  if (!m_hTexture.IsInvalidated() && ezGALDevice::GetDefaultDevice()->GetTexture(m_hTexture) != nullptr)
  {
    ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
    bindGroup.BindTexture("ScreenSpaceShadowTexture", m_hTexture);
  }
}

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScreenSpaceShadowDataProvider, 1, ezRTTIDefaultAllocator<ezScreenSpaceShadowDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScreenSpaceShadowDataProvider::ezScreenSpaceShadowDataProvider() = default;

ezScreenSpaceShadowDataProvider::~ezScreenSpaceShadowDataProvider() = default;

void ezScreenSpaceShadowDataProvider::SetTexture(ezGALTextureHandle hTexture)
{
  m_Data.m_hTexture = hTexture;
}

void* ezScreenSpaceShadowDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  return &m_Data;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ScreenSpaceShadowDataProvider);
