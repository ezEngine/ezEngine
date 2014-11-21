#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>

void ezViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  if (m_Window.m_hWnd != 0)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_Window.m_hWnd = hWnd;
  m_Window.m_uiWidth = uiWidth;
  m_Window.m_uiHeight = uiHeight;

  {
    ezGALSwapChainCreationDescription scd;
    scd.m_pWindow = &m_Window;
    scd.m_SampleCount = ezGALMSAASampleCount::None;
    scd.m_bCreateDepthStencilBuffer = true;
    scd.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
    scd.m_bAllowScreenshots = true;
    scd.m_bVerticalSynchronization = true;

    m_hPrimarySwapChain = pDevice->CreateSwapChain(scd);
    const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(m_hPrimarySwapChain);
    EZ_ASSERT(pPrimarySwapChain != nullptr, "Failed to init swapchain");

    m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
    EZ_ASSERT(!m_hBBRT.IsInvalidated(), "Failed to init render target");
  }

  ezGALRasterizerStateCreationDescription RasterStateDesc;
  RasterStateDesc.m_bWireFrame = true;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerState = pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");

  ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
  DepthStencilStateDesc.m_bDepthTest = true;
  DepthStencilStateDesc.m_bDepthWrite = true;
  m_hDepthStencilState = pDevice->CreateDepthStencilState(DepthStencilStateDesc);
  EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");

}

