
#pragma once

ezUInt8 ezGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

ezGALRenderTargetViewHandle ezGALRenderTargetSetup::GetRenderTarget(ezUInt8 uiIndex) const
{
  EZ_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_hRTs[uiIndex];
}

ezGALRenderTargetViewHandle ezGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
