#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"
#include <Core/Graphics/Camera.h>

ezTestAppRun ezRendererTestBasics::SubtestRasterizerStates()
{
  BeginFrame();
  BeginCommands("RasterizerStates");
  ezGALRasterizerStateHandle hState;

  ezGALRasterizerStateCreationDescription RasterStateDesc;

  if (m_iFrame == 0)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::None;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 1)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 2)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 3)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 4)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 5)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 6)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = false;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = true;
  }

  if (m_iFrame == 7)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::None;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 8)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 9)
  {
    RasterStateDesc.m_bFrontCounterClockwise = false;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 10)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bScissorTest = false;
  }

  if (m_iFrame == 11)
  {
    RasterStateDesc.m_bFrontCounterClockwise = true;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Front;
    RasterStateDesc.m_bScissorTest = false;
  }

  ezColor clear(0, 0, 0, 0);
  if (!RasterStateDesc.m_bFrontCounterClockwise)
    clear.g = 0.5f;
  if (RasterStateDesc.m_CullMode == ezGALCullMode::Front)
    clear.b = 0.5f;
  if (RasterStateDesc.m_CullMode == ezGALCullMode::Back)
    clear.b = 1.0f;

  BeginRendering(clear);

  hState = m_pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create rasterizer state!");

  ezRenderContext::GetDefaultInstance()->SetRasterizerState(hState);

  ezRenderContext::GetDefaultInstance()->GetCommandEncoder()->SetScissorRect(ezRectU32(100, 50, GetResolution().width / 2, GetResolution().height / 2));

  RenderObjects(ezShaderBindFlags::NoRasterizerState);

  EndRendering();
  if (RasterStateDesc.m_bWireFrame)
  {
    ezStringView sRendererName = m_pDevice->GetRenderer();
    const bool bRandomlyChangesLineThicknessOnDriverUpdate = sRendererName.IsEqual_NoCase("DX11") && m_pDevice->GetCapabilities().m_sAdapterName.FindSubString_NoCase("Nvidia");

    EZ_TEST_LINE_IMAGE(m_iFrame, bRandomlyChangesLineThicknessOnDriverUpdate ? 1000 : 300);
  }
  else
    EZ_TEST_IMAGE(m_iFrame, 200);
  EndCommands();
  EndFrame();

  m_pDevice->DestroyRasterizerState(hState);

  return m_iFrame < 11 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}
