#include <EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

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

  if (m_pActor != nullptr)
    return;

  ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "Engine View", this);
  m_pActor = pActor.Borrow();

  // create window
  {
    ezUniquePtr<ezRemoteProcessWindow> pWindow = EZ_DEFAULT_NEW(ezRemoteProcessWindow);

    ezWindowCreationDesc desc;
    desc.m_uiWindowNumber = 0;
    desc.m_bClipMouseCursor = false;
    desc.m_bShowMouseCursor = true;
    desc.m_Resolution = ezSizeU32(1024, 768);
    desc.m_WindowMode = ezWindowMode::WindowFixedResolution;
    desc.m_Title = "Engine View";

    pWindow->Initialize(desc);

    pActor->m_pWindow = std::move(pWindow);
  }

  ezActorManager::GetSingleton()->AddActor(std::move(pActor));
}

void ezEditorEngineProcessApp::DestroyRemoteWindow()
{
  if (!m_hRemoteView.IsInvalidated())
  {
    ezRenderWorld::DeleteView(m_hRemoteView);
    m_hRemoteView.Invalidate();
  }

  if (ezActorManager::GetSingleton())
  {
    ezActorManager::GetSingleton()->DestroyAllActors(this);
  }

  m_pActor = nullptr;
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
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    // create output target
    {
      ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);

      ezGALSwapChainCreationDescription desc;
      desc.m_pWindow = m_pActor->m_pWindow.Borrow();
      desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      desc.m_bAllowScreenshots = true;

      pOutput->CreateSwapchain(desc);

      m_pActor->m_pWindowOutputTarget = std::move(pOutput);
    }

    // create render target
    ezGALRenderTargetSetup BackBufferRenderTargetSetup;
    {
      ezWindowOutputTargetGAL* pOutputTarget = static_cast<ezWindowOutputTargetGAL*>(m_pActor->m_pWindowOutputTarget.Borrow());
      const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(pOutputTarget->m_hSwapChain);
      EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

      auto hSwapChainRTV = pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());

      BackBufferRenderTargetSetup.SetRenderTarget(0, hSwapChainRTV);
    }

    // setup view
    {
      ezView* pView = nullptr;
      m_hRemoteView = ezRenderWorld::CreateView("Remote Process", pView);

      // EditorRenderPipeline.ezRenderPipelineAsset
      pView->SetRenderPipelineResource(
        ezResourceManager::LoadResource<ezRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

      const ezSizeU32 wndSize = m_pActor->m_pWindow->GetClientAreaSize();

      pView->SetRenderTargetSetup(BackBufferRenderTargetSetup);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)wndSize.width, (float)wndSize.height));
      pView->SetCamera(pCamera);
    }
  }

  return m_hRemoteView;
}
