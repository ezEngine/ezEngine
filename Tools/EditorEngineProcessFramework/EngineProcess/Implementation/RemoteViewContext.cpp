#include <PCH.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererFoundation/Device/Device.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/RenderPipelineResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Pipeline/View.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

ezUniquePtr<ezRemoteProcessWindow> ezRemoteEngineProcessViewContext::s_pCustomWindow;
ezVec2I32 ezRemoteEngineProcessViewContext::s_WindowPosition(-1, -1);

ezViewHandle ezRemoteEngineProcessViewContext::s_hView;
ezUInt32 ezRemoteEngineProcessViewContext::s_uiActiveViewID = 0;
ezInt32 ezRemoteEngineProcessViewContext::s_iWindowReferences = 0;

ezRemoteEngineProcessViewContext::ezRemoteEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
  : ezEngineProcessViewContext(pContext)
{

}

ezRemoteEngineProcessViewContext::~ezRemoteEngineProcessViewContext()
{
  // make sure this isn't destroyed here
  if (!m_hView.IsInvalidated())
  {
    m_hView.Invalidate();

    DestroyWindowAndView();
  }
}

void ezRemoteEngineProcessViewContext::CreateWindowAndView()
{
  ++s_iWindowReferences;

  if (s_pCustomWindow != nullptr)
    return;

  s_pCustomWindow = EZ_DEFAULT_NEW(ezRemoteProcessWindow);

  ezWindowCreationDesc desc;
  desc.m_uiWindowNumber = 0;
  desc.m_bClipMouseCursor = false;
  desc.m_bShowMouseCursor = true;
  desc.m_Resolution = ezSizeU32(600, 600);
  desc.m_WindowMode = ezWindowMode::WindowFixedResolution;
  desc.m_Title = "Engine View";

  if (s_WindowPosition.x != -1)
  {
    desc.m_Position = s_WindowPosition;
  }

  s_pCustomWindow->Initialize(desc);
  s_pCustomWindow->m_iWindowPosX = desc.m_Position.x;
  s_pCustomWindow->m_iWindowPosY = desc.m_Position.y;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto hPrimarySwapChain = static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->AddWindow(s_pCustomWindow.Borrow());
  const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(hPrimarySwapChain);
  EZ_ASSERT_DEV(pPrimarySwapChain != nullptr, "Failed to init swapchain");

  auto hSwapChainRTV = pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());

  ezGALRenderTagetSetup BackBufferRenderTargetSetup;
  BackBufferRenderTargetSetup.SetRenderTarget(0, hSwapChainRTV);

  // setup view
  {
    ezView* pView = nullptr;
    s_hView = ezRenderWorld::CreateView("Remote Process", pView);

    // default render pipeline
    pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>("{ da463c4d-c984-4910-b0b7-a0b3891d0448 }"));

    pView->SetRenderTargetSetup(BackBufferRenderTargetSetup);
    pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)desc.m_Resolution.width, (float)desc.m_Resolution.height));
  }
}


void ezRemoteEngineProcessViewContext::DestroyWindowAndView()
{
  --s_iWindowReferences;

  if (s_iWindowReferences > 0)
    return;

  if (!s_hView.IsInvalidated())
  {
    ezRenderWorld::DeleteView(s_hView);
  }

  if (s_pCustomWindow->IsInitialized())
  {
    s_WindowPosition.Set(s_pCustomWindow->m_iWindowPosX, s_pCustomWindow->m_iWindowPosY);

    static_cast<ezGameApplication*>(ezApplication::GetApplicationInstance())->RemoveWindow(s_pCustomWindow.Borrow());

    s_pCustomWindow.Reset();
  }
}

void ezRemoteEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      CreateWindowAndView();
      m_hView = s_hView;
    }

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);
    Redraw(false);
  }
}

ezViewHandle ezRemoteEngineProcessViewContext::CreateView()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezViewHandle();
}

void ezRemoteProcessWindow::OnWindowMoveMessage(const ezInt32 newPosX, const ezInt32 newPosY)
{
  m_iWindowPosX = newPosX;
  m_iWindowPosY = newPosY;
}
