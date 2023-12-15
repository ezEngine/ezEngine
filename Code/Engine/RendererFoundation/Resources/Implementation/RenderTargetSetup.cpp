#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

bool ezGALRenderTargets::operator==(const ezGALRenderTargets& other) const
{
  if (m_hDSTarget != other.m_hDSTarget)
    return false;

  for (ezUInt8 uiRTIndex = 0; uiRTIndex < EZ_GAL_MAX_RENDERTARGET_COUNT; ++uiRTIndex)
  {
    if (m_hRTs[uiRTIndex] != other.m_hRTs[uiRTIndex])
      return false;
  }
  return true;
}

bool ezGALRenderTargets::operator!=(const ezGALRenderTargets& other) const
{
  return !(*this == other);
}

ezGALRenderTargetSetup::ezGALRenderTargetSetup() = default;

ezGALRenderTargetSetup& ezGALRenderTargetSetup::SetRenderTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget)
{
  EZ_ASSERT_DEV(uiIndex < EZ_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than EZ_GAL_MAX_RENDERTARGET_COUNT");

  m_hRTs[uiIndex] = hRenderTarget;

  m_uiRTCount = ezMath::Max(m_uiRTCount, static_cast<ezUInt8>(uiIndex + 1u));

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

  if (m_uiRTCount != other.m_uiRTCount)
    return false;

  for (ezUInt8 uiRTIndex = 0; uiRTIndex < m_uiRTCount; ++uiRTIndex)
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
  m_uiRTCount = 0;
}

bool ezGALRenderingSetup::operator==(const ezGALRenderingSetup& other) const
{
  return m_RenderTargetSetup == other.m_RenderTargetSetup && m_ClearColor == other.m_ClearColor && m_uiRenderTargetClearMask == other.m_uiRenderTargetClearMask && m_fDepthClear == other.m_fDepthClear && m_uiStencilClear == other.m_uiStencilClear && m_bClearDepth == other.m_bClearDepth && m_bClearStencil == other.m_bClearStencil && m_bDiscardColor == other.m_bDiscardColor && m_bDiscardDepth == other.m_bDiscardDepth;
}

bool ezGALRenderingSetup::operator!=(const ezGALRenderingSetup& other) const
{
  return !(*this == other);
}


