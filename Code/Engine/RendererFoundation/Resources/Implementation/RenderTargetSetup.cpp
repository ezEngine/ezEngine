
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezGALRenderTagetSetup::ezGALRenderTagetSetup()
  : m_uiMaxRTIndex( 0xFFu )
{
}

ezGALRenderTagetSetup& ezGALRenderTagetSetup::SetRenderTarget(ezUInt8 uiIndex, ezGALRenderTargetViewHandle hRenderTarget)
{
  EZ_ASSERT_DEV( uiIndex < EZ_GAL_MAX_RENDERTARGET_COUNT, "Render target index out of bounds - should be less than EZ_GAL_MAX_RENDERTARGET_COUNT" );

  m_hRTs[uiIndex] = hRenderTarget;

  if ( !HasRenderTargets() )
  {
    m_uiMaxRTIndex = uiIndex;
  }
  else
  {
    m_uiMaxRTIndex = ezMath::Max( m_uiMaxRTIndex, uiIndex );
  }

  return *this;
}

ezGALRenderTagetSetup& ezGALRenderTagetSetup::SetDepthStencilTarget(ezGALRenderTargetViewHandle hDSTarget)
{
  m_hDSTarget = hDSTarget;

  return *this;
}

bool ezGALRenderTagetSetup::operator == (const ezGALRenderTagetSetup& Other) const
{
  if ( m_hDSTarget != Other.m_hDSTarget )
    return false;
  
  if ( m_uiMaxRTIndex != Other.m_uiMaxRTIndex )
    return false;

  for ( ezUInt8 uiRTIndex = 0; uiRTIndex < m_uiMaxRTIndex; ++uiRTIndex )
  {
    if ( m_hRTs[uiRTIndex] != Other.m_hRTs[uiRTIndex] )
      return false;
  }

  return true;
}

EZ_STATICLINK_FILE( RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetSetup );