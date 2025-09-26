#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

EZ_IMPLEMENT_SINGLETON(ezEditorEngineProcessApp);

ezEditorEngineProcessApp::ezEditorEngineProcessApp()
  : m_SingletonRegistrar(this)
{
}

ezEditorEngineProcessApp::~ezEditorEngineProcessApp()
{
  DestroyRemoteWindow();
}

void ezEditorEngineProcessApp::SetRemoteMode()
{
  m_Mode = ezEditorEngineProcessMode::Remote;

  CreateRemoteWindow();
}

void ezEditorEngineProcessApp::CreateRemoteWindow()
{
  EZ_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  if (!m_hWindow.IsInvalidated())
    return;

  ezUniquePtr<ezRemoteProcessWindow> pWindow = EZ_DEFAULT_NEW(ezRemoteProcessWindow);

  ezWindowCreationDesc desc;
  desc.m_uiWindowNumber = 0;
  desc.m_bClipMouseCursor = false;
  desc.m_bShowMouseCursor = true;
  desc.m_Resolution = ezSizeU32(1024, 768);
  desc.m_WindowMode = ezWindowMode::WindowFixedResolution;
  desc.m_Title = "Engine View";

  pWindow->Initialize(desc).IgnoreResult();

  m_hWindow = ezWindowManager::GetSingleton()->Register("Engine View", this, std::move(pWindow));
}

void ezEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (!m_hRemoteView.IsInvalidated())
  {
    ezRenderWorld::DeleteView(m_hRemoteView);
    m_hRemoteView.Invalidate();
  }

  if (ezWindowManager::GetSingleton())
  {
    ezWindowManager::GetSingleton()->CloseAll(this);
  }

  m_hWindow.Invalidate();
}

ezRenderPipelineResourceHandle ezEditorEngineProcessApp::CreateDefaultMainRenderPipeline()
{
  // EditorRenderPipeline.ezRenderPipelineAsset
  return ezResourceManager::LoadResource<ezRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }");
}

ezRenderPipelineResourceHandle ezEditorEngineProcessApp::CreateDefaultDebugRenderPipeline()
{
  // DebugRenderPipeline.ezRenderPipelineAsset
  return ezResourceManager::LoadResource<ezRenderPipelineResource>("{ 0416eb3e-69c0-4640-be5b-77354e0e37d7 }");
}

ezViewHandle ezEditorEngineProcessApp::CreateRemoteWindowAndView(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(IsRemoteMode(), "Incorrect app mode");

  CreateRemoteWindow();

  if (m_hRemoteView.IsInvalidated())
  {
    auto pWinMan = ezWindowManager::GetSingleton();

    // create output target
    {
      ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);

      ezGALWindowSwapChainCreationDescription desc;
      desc.m_pWindow = pWinMan->GetWindow(m_hWindow);
      desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;

      pOutput->CreateSwapchain(desc);

      pWinMan->SetOutputTarget(m_hWindow, std::move(pOutput));
    }

    // get swapchain
    ezGALSwapChainHandle hSwapChain;
    {
      ezWindowOutputTargetGAL* pOutputTarget = static_cast<ezWindowOutputTargetGAL*>(pWinMan->GetOutputTarget(m_hWindow));
      hSwapChain = pOutputTarget->m_hSwapChain;
    }

    // setup view
    {
      ezView* pView = nullptr;
      m_hRemoteView = ezRenderWorld::CreateView("Remote Process", pView);

      // EditorRenderPipeline.ezRenderPipelineAsset
      pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

      const ezSizeU32 wndSize = pWinMan->GetWindow(m_hWindow)->GetClientAreaSize();

      pView->SetSwapChain(hSwapChain);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)wndSize.width, (float)wndSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
