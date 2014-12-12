#include <PCH.h>
#include "Basics.h"
#include <RendererFoundation/Context/Context.h>
#include <CoreUtils/Graphics/Camera.h>

ezTestAppRun ezRendererTestBasics::SubtestRasterizerStates()
{
  BeginFrame();

  ezGALRasterizerStateHandle hState;

  ezUInt32 state = m_uiFrame;

  ezGALRasterizerStateCreationDescription RasterStateDesc;
  RasterStateDesc.m_bDepthClip = (state % 2) == 0; state /= 2;
  RasterStateDesc.m_bFrontCounterClockwise = (state % 2) == 0; state /= 2;
  RasterStateDesc.m_bWireFrame = (state % 2) == 1; state /= 2;
  RasterStateDesc.m_CullMode = (ezGALCullMode::Enum) (ezGALCullMode::None + (state % 3)); state /= 3;
  RasterStateDesc.m_bScissorTest = (state % 2) == 1; state /= 2;

  ezColor clear(0, 0, 0, 0);
  if (RasterStateDesc.m_bDepthClip)
    clear.r = 0.5f;
  if (RasterStateDesc.m_bFrontCounterClockwise)
    clear.g = 0.5f;
  if (RasterStateDesc.m_CullMode == ezGALCullMode::Front)
    clear.b = 0.5f;
  if (RasterStateDesc.m_CullMode == ezGALCullMode::Back)
    clear.b = 1.0f;

  ClearScreen(clear);

  hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT(!hState.IsInvalidated(), "Couldn't create rasterizer state!");

  if (hState.IsInvalidated())
  {
    int i = 0;
  }

  m_pDevice->GetPrimaryContext()->SetRasterizerState(hState);

  m_pDevice->GetPrimaryContext()->SetScissorRect(100, 50, GetResolution().width - 150, GetResolution().height - 20);

  RenderObjects();

  EZ_TEST_IMAGE(0);

  EndFrame();

  m_pDevice->DestroyRasterizerState(hState);

  return m_uiFrame < 48 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

