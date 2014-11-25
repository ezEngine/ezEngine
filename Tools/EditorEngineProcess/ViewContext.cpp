#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>

ezMeshBufferResourceHandle CreateTranslateGizmo();

void ezViewContext::SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  if (GetEditorWindow().m_hWnd != 0)
  {
    if (GetEditorWindow().m_uiWidth == uiWidth && GetEditorWindow().m_uiHeight == uiHeight)
      return;

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    pDevice->DestroySwapChain(m_hPrimarySwapChain);
    m_hPrimarySwapChain.Invalidate();
  }
  else
  {
    m_hSphere = DontUse::CreateSphere(3);
    m_hTranslateGizmo = CreateTranslateGizmo();
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  GetEditorWindow().m_hWnd = hWnd;
  GetEditorWindow().m_uiWidth = uiWidth;
  GetEditorWindow().m_uiHeight = uiHeight;

  ezLog::Debug("Creating Swapchain with size %u * %u", uiWidth, uiHeight);

  {
    ezGALSwapChainCreationDescription scd;
    scd.m_pWindow = &GetEditorWindow();
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

  RasterStateDesc.m_bWireFrame = false;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerStateGizmo = pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");
  

  ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
  DepthStencilStateDesc.m_bDepthTest = true;
  DepthStencilStateDesc.m_bDepthWrite = true;
  m_hDepthStencilState = pDevice->CreateDepthStencilState(DepthStencilStateDesc);
  EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");


  // Create a constant buffer for matrix upload
  m_hCB = pDevice->CreateConstantBuffer(sizeof(ezMat4));

  ezShaderManager::SetPlatform("DX11_SM40", pDevice, true);

  m_hShader = ezResourceManager::GetResourceHandle<ezShaderResource>("Shaders/Wireframe.shader");
  m_hGizmoShader = ezResourceManager::GetResourceHandle<ezShaderResource>("Shaders/Gizmo.shader");
}

