
#pragma once

bool ezGALRenderTagetSetup::HasRenderTargets() const
{
  return m_uiMaxRTIndex != 0xFFu;
}

ezUInt8 ezGALRenderTagetSetup::GetMaxRenderTargetIndex() const
{
  return m_uiMaxRTIndex;
}

ezGALRenderTargetViewHandle ezGALRenderTagetSetup::GetRenderTarget( ezUInt8 uiIndex ) const
{
  EZ_ASSERT_DEBUG( uiIndex <= m_uiMaxRTIndex, "Render target index out of range" );

  return m_hRTs[uiIndex];
}

ezGALRenderTargetViewHandle ezGALRenderTagetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
