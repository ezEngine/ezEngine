#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <RendererTest/Basics/SwapChain.h>

ezResult ezRendererTestSwapChain::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;

  if (ezGraphicsTest::InitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  if (SetupRenderer().Failed())
    return EZ_FAILURE;

  m_CurrentWindowSize = ezSizeU32(320, 240);

  // Window
  {
    ezWindowCreationDesc WindowCreationDesc;
    WindowCreationDesc.m_Resolution.width = m_CurrentWindowSize.width;
    WindowCreationDesc.m_Resolution.height = m_CurrentWindowSize.height;
    WindowCreationDesc.m_WindowMode = iIdentifier == SubTests::ST_ResizeWindow ? ezWindowMode::WindowResizable : ezWindowMode::WindowFixedResolution;
    // ezGameStateWindow will write any window size changes into the config.
    m_pWindow = EZ_DEFAULT_NEW(ezGameStateWindow, WindowCreationDesc);
  }

  // SwapChain
  {
    ezGALWindowSwapChainCreationDescription swapChainDesc;
    swapChainDesc.m_pWindow = m_pWindow;
    swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
    swapChainDesc.m_bAllowScreenshots = true;
    swapChainDesc.m_PresentMode = SubTests::ST_NoVSync ? ezGALPresentMode::Immediate : ezGALPresentMode::VSync;
    m_hSwapChain = ezGALWindowSwapChain::Create(swapChainDesc);
  }

  // Depth Texture
  if (iIdentifier != SubTests::ST_ColorOnly)
  {
    ezGALTextureCreationDescription texDesc;
    texDesc.m_uiWidth = m_CurrentWindowSize.width;
    texDesc.m_uiHeight = m_CurrentWindowSize.height;
    switch (iIdentifier)
    {
      case SubTests::ST_D16:
        texDesc.m_Format = ezGALResourceFormat::D16;
        break;
      case SubTests::ST_D24S8:
        texDesc.m_Format = ezGALResourceFormat::D24S8;
        break;
      default:
      case SubTests::ST_D32:
        texDesc.m_Format = ezGALResourceFormat::DFloat;
        break;
    }

    texDesc.m_bCreateRenderTarget = true;
    m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestSwapChain::DeInitializeSubTest(ezInt32 iIdentifier)
{
  DestroyWindow();
  ShutdownRenderer();

  if (ezGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}


void ezRendererTestSwapChain::ResizeTest(ezUInt32 uiInvocationCount)
{
  if (uiInvocationCount == 4)
  {
    // Not implemented on all platforms,  so we ignore the result here.
    m_pWindow->Resize(ezSizeU32(640, 480)).IgnoreResult();
  }

  if (m_pWindow->GetClientAreaSize() != m_CurrentWindowSize)
  {
    m_CurrentWindowSize = m_pWindow->GetClientAreaSize();
    m_pDevice->DestroySwapChain(m_hSwapChain);
    m_hSwapChain.Invalidate();

    m_pDevice->DestroyTexture(m_hDepthStencilTexture);
    m_hDepthStencilTexture.Invalidate();

    // Swap Chain
    {
      ezGALWindowSwapChainCreationDescription swapChainDesc;
      swapChainDesc.m_pWindow = m_pWindow;
      swapChainDesc.m_SampleCount = ezGALMSAASampleCount::None;
      swapChainDesc.m_bAllowScreenshots = true;
      swapChainDesc.m_PresentMode = SubTests::ST_NoVSync ? ezGALPresentMode::Immediate : ezGALPresentMode::VSync;
      m_hSwapChain = ezGALWindowSwapChain::Create(swapChainDesc);
    }

    // Depth Texture
    {
      ezGALTextureCreationDescription texDesc;
      texDesc.m_uiWidth = m_CurrentWindowSize.width;
      texDesc.m_uiHeight = m_CurrentWindowSize.height;
      texDesc.m_Format = ezGALResourceFormat::DFloat;
      texDesc.m_bCreateRenderTarget = true;
      m_hDepthStencilTexture = m_pDevice->CreateTexture(texDesc);
    }
  }
}

ezTestAppRun ezRendererTestSwapChain::BasicRenderLoop(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_pDevice->BeginFrame();
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);
  m_pPass = m_pDevice->BeginPass("SwapChainTest");
  {
    const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
    renderingSetup.m_ClearColor = ezColor::CornflowerBlue;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    if (!m_hDepthStencilTexture.IsInvalidated())
    {
      renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
      renderingSetup.m_bClearDepth = true;
      renderingSetup.m_bClearStencil = true;
    }
    ezRectFloat viewport = ezRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
    m_pWindow->ProcessWindowMessages();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();

  return m_iFrame < 120 ? ezTestAppRun::Continue : ezTestAppRun::Quit;
}

static ezRendererTestSwapChain g_SwapChainTest;
