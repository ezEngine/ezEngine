#include <PCH.h>
#include "Basics.h"

ezTestAppRun ezRendererTestBasics::SubtestBlendStates()
{
  BeginFrame();

  ezGALBlendStateHandle hState;

  ezGALBlendStateCreationDescription StateDesc;
  StateDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = true;

  if (m_iFrame == 0)
  {
    //StateDesc.m_RenderTargetBlendDescriptions[0].
  }

  if (m_iFrame == 1)
  {
    StateDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend = ezGALBlend::SrcAlpha;
    StateDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend = ezGALBlend::InvSrcAlpha;
  }

  ezColor clear(0, 0, 0, 0);
  //if (StateDesc.m_bDepthClip)
  //  clear.r = 0.5f;
  //if (StateDesc.m_bFrontCounterClockwise)
  //  clear.g = 0.5f;
  //if (StateDesc.m_CullMode == ezGALCullMode::Front)
  //  clear.b = 0.5f;
  //if (StateDesc.m_CullMode == ezGALCullMode::Back)
  //  clear.b = 1.0f;

  ClearScreen(clear);

  hState = m_pDevice->CreateBlendState(StateDesc);
  EZ_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create blend state!");

  m_pDevice->GetPrimaryContext()->SetBlendState(hState);

  RenderObjects(ezShaderBindFlags::NoBlendState);

  EZ_TEST_IMAGE(150);

  EndFrame();

  m_pDevice->DestroyBlendState(hState);

  return m_iFrame < 1 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}



