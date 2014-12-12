#include <PCH.h>
#include "Basics.h"
#include <RendererFoundation/Context/Context.h>
#include <CoreUtils/Graphics/Camera.h>

ezTestAppRun ezRendererTestBasics::SubtestRasterizerStates()
{
  BeginFrame();
  ClearScreen(ezColor::GetCornflowerBlue()); // The original!

  ezCamera cam;
  cam.SetCameraMode(ezCamera::PerspectiveFixedFovX, 90, 0.1f, 1000.0f);
  cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1));
  ezMat4 mProj, mView;
  cam.GetProjectionMatrix(16.0f / 9.0f, ezProjectionDepthRange::ZeroToOne, mProj);
  cam.GetViewMatrix(mView);

  ezGALRasterizerStateHandle hState;

  if (m_uiFrame == 1)
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::None;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT(!hState.IsInvalidated(), "Couldn't create rasterizer state!");
  }

  if (m_uiFrame == 2)
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Front;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT(!hState.IsInvalidated(), "Couldn't create rasterizer state!");
  }

  if (m_uiFrame == 3)
  {
    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT(!hState.IsInvalidated(), "Couldn't create rasterizer state!");
  }

  m_pDevice->GetPrimaryContext()->SetRasterizerState(hState);

  ezMat4 mTransform;
  mTransform.SetIdentity();
  mTransform.SetTranslationMatrix(ezVec3(0, 0, -2.0f));

  mTransform = mProj * mView * mTransform;

  RenderObject(m_hSphere, mTransform);

  EZ_TEST_IMAGE(0);

  EndFrame();

  m_pDevice->DestroyRasterizerState(hState);

  return m_uiFrame < 3 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

