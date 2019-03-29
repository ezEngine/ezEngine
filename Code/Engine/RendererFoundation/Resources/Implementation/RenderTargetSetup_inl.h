
#pragma once

bool ezGALRenderTargetSetup::HasRenderTargets() const
{
  return m_uiMaxRTIndex != 0xFFu;
}

ezUInt8 ezGALRenderTargetSetup::GetMaxRenderTargetIndex() const
{
  return m_uiMaxRTIndex;
}

ezGALRenderTargetViewHandle ezGALRenderTargetSetup::GetRenderTarget( ezUInt8 uiIndex ) const
{
  EZ_ASSERT_DEBUG( uiIndex <= m_uiMaxRTIndex, "Render target index out of range" );

  return m_hRTs[uiIndex];
}

ezGALRenderTargetViewHandle ezGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}

