#include <RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezGALRenderTargetSetup::ezGALRenderTargetSetup()
  : m_uiMaxRTIndex(0xFFu)
{
}

ezGALRenderTargetSetup& ezGALRenderTargetSetup::SetRenderTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget)
{
  EZ_ASSERT_DEV(
    uiIndex < EZ_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than EZ_GAL_MAX_RENDERTARGET_COUNT");

  m_hRTs[uiIndex] = hRenderTarget;

  if (!HasRenderTargets())
  {
    m_uiMaxRTIndex = uiIndex;
  }
  else
  {
    m_uiMaxRTIndex = ezMath::Max(m_uiMaxRTIndex, uiIndex);
  }

  return *this;
}

ezGALRenderTargetSetup& ezGALRenderTargetSetup::SetDepthStencilTarget(ezGALRenderTargetViewHandle hDSTarget)
{
  m_hDSTarget = hDSTarget;

  return *this;
}

bool ezGALRenderTargetSetup::operator==(const ezGALRenderTargetSetup& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  if (m_uiMaxRTIndex != other.m_uiMaxRTIndex)
    return false;

  for (ezUInt8 uiRTIndex = 0; uiRTIndex <= m_uiMaxRTIndex; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }

  return true;
}

bool ezGALRenderTargetSetup::operator!=(const ezGALRenderTargetSetup& other) const
{
  return !(*this == other);
}

void ezGALRenderTargetSetup::DestroyAllAttachedViews()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezArrayPtr<ezGALRenderTargetViewHandle> colorViews(m_hRTs);
  for (ezGALRenderTargetViewHandle& hView : colorViews)
  {
    if (!hView.IsInvalidated())
    {
      pDevice->DestroyRenderTargetView(hView);
      hView.Invalidate();
    }
  }

  if (!m_hDSTarget.IsInvalidated())
  {
    pDevice->DestroyRenderTargetView(m_hDSTarget);
    m_hDSTarget.Invalidate();
  }
  m_uiMaxRTIndex = 0xFFu;
}

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetSetup);
