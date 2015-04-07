#include <PCH.h>
#include <EditorEngineProcess/ViewContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>

ezMeshBufferResourceHandle CreateTranslateGizmoMesh();

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
    m_hSphere = DontUse::CreateSphereMesh(3);
    m_hTranslateGizmo = CreateTranslateGizmoMesh();
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
    EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

    m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
    EZ_ASSERT_DEV(!m_hBBRT.IsInvalidated(), "Failed to init render target");
  }

  ezGALRasterizerStateCreationDescription RasterStateDesc;
  RasterStateDesc.m_bWireFrame = true;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerState = pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT_DEV(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");

  RasterStateDesc.m_bWireFrame = false;
  RasterStateDesc.m_CullMode = ezGALCullMode::Back;
  RasterStateDesc.m_bFrontCounterClockwise = true;
  m_hRasterizerStateGizmo = pDevice->CreateRasterizerState(RasterStateDesc);
  EZ_ASSERT_DEV(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");
  

  ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
  DepthStencilStateDesc.m_bDepthTest = true;
  DepthStencilStateDesc.m_bDepthWrite = true;
  m_hDepthStencilState = pDevice->CreateDepthStencilState(DepthStencilStateDesc);
  EZ_ASSERT_DEV(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");


  // Create a constant buffer for matrix upload
  m_hCB = pDevice->CreateConstantBuffer(sizeof(ObjectData));

  ezRenderContext::ConfigureShaderSystem("DX11_SM40", true);

  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Wireframe.ezShader");
  m_hGizmoShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Gizmo.ezShader");

  // Create render target for picking
  {
    if (!m_hPickingRT.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hPickingRT);
      m_hPickingRT.Invalidate();
    }

    if (!m_hPickingDepthRT.IsInvalidated())
    {
      pDevice->DestroyTexture(m_hPickingDepthRT);
      m_hPickingDepthRT.Invalidate();
    }

    if (!m_hPickingRenderTargetCfg.IsInvalidated())
    {
      pDevice->DestroyRenderTargetConfig(m_hPickingRenderTargetCfg);
      m_hPickingRenderTargetCfg.Invalidate();
    }

    ezGALTextureCreationDescription tcd;
    tcd.m_bAllowDynamicMipGeneration = false;
    tcd.m_bAllowShaderResourceView = false;
    tcd.m_bAllowUAV = false;
    tcd.m_bCreateRenderTarget = true;
    tcd.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
    tcd.m_ResourceAccess.m_bReadBack = true;
    tcd.m_Type = ezGALTextureType::Texture2D;
    tcd.m_uiWidth = uiWidth;
    tcd.m_uiHeight = uiHeight;

    m_hPickingRT = pDevice->CreateTexture(tcd);

    tcd.m_Format = ezGALResourceFormat::D24S8;
    tcd.m_ResourceAccess.m_bReadBack = false;

    m_hPickingDepthRT = pDevice->CreateTexture(tcd);

    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = m_hPickingRT;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;
    ezGALRenderTargetViewHandle hRTVcol = pDevice->CreateRenderTargetView(rtvd);

    rtvd.m_hTexture = m_hPickingDepthRT;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;
    ezGALRenderTargetViewHandle hRTVdepth = pDevice->CreateRenderTargetView(rtvd);

    ezGALRenderTargetConfigCreationDescription rtd;
    rtd.m_bHardwareBackBuffer = false;
    rtd.m_hColorTargets[0] = hRTVcol;
    rtd.m_hDepthStencilTarget = hRTVdepth;
    rtd.m_uiColorTargetCount = 1;

    m_hPickingRenderTargetCfg = pDevice->CreateRenderTargetConfig(rtd);

    m_PickingRenderTargetDT.EnableDataTransfer("Picking RT");
  }

}

