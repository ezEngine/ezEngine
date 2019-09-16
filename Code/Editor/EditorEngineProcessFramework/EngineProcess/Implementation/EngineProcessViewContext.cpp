#include <EditorEngineProcessFrameworkPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/ActorSystem/Actor.h>
#include <GameEngine/ActorSystem/ActorManager.h>
#include <GameEngine/ActorSystem/ActorPluginWindow.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

ezEngineProcessViewContext::ezEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
  : m_pDocumentContext(pContext)
{
  m_uiViewID = 0xFFFFFFFF;
}

ezEngineProcessViewContext::~ezEngineProcessViewContext()
{
  ezRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();

  ezActorManager::GetSingleton()->DestroyAllActors(this);
}

void ezEngineProcessViewContext::SetViewID(ezUInt32 id)
{
  EZ_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once");
  m_uiViewID = id;
}

void ezEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
      HandleWindowUpdate(reinterpret_cast<ezWindowHandle>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
      Redraw(true);
    }
  }
  else if (ezViewScreenshotMsgToEngine* msg = ezDynamicCast<ezViewScreenshotMsgToEngine*>(pMsg))
  {
    ezImage img;
    ezActorPluginWindow* pWindow = m_pEditorWndActor->GetPlugin<ezActorPluginWindow>();
    pWindow->GetOutputTarget()->CaptureImage(img);

    img.SaveTo(msg->m_sOutputFile);
  }
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  EZ_REPORT_FAILURE("This code path should never be executed on UWP.");
#elif
#  error "Unsupported platform."
#endif
}

void ezEngineProcessViewContext::SendViewMessage(ezEditorEngineViewMsg* pViewMsg)
{
  pViewMsg->m_DocumentGuid = GetDocumentContext()->GetDocumentGuid();
  pViewMsg->m_uiViewID = m_uiViewID;

  GetDocumentContext()->SendProcessMessage(pViewMsg);
}

void ezEngineProcessViewContext::HandleWindowUpdate(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezEngineProcessViewContext::HandleWindowUpdate");

  if (m_pEditorWndActor != nullptr)
  {
    ezActorPluginWindow* pWindowPlugin = m_pEditorWndActor->GetPlugin<ezActorPluginWindow>();

    const ezSizeU32 wndSize = pWindowPlugin->GetWindow()->GetClientAreaSize();

    if (wndSize.width == uiWidth && wndSize.height == uiHeight)
      return;

    ezActorManager::GetSingleton()->DestroyActor(m_pEditorWndActor);
    m_pEditorWndActor = nullptr;
  }

  {
    ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "EditorView", this);
    m_pEditorWndActor = pActor.Borrow();

    ezUniquePtr<ezActorPluginWindowOwner> pWindowPlugin = EZ_DEFAULT_NEW(ezActorPluginWindowOwner);

    // create window
    {
      ezUniquePtr<ezEditorProcessViewWindow> pWindow = EZ_DEFAULT_NEW(ezEditorProcessViewWindow);
      pWindow->m_hWnd = hWnd;
      pWindow->m_uiWidth = 640;//uiWidth;
      pWindow->m_uiHeight = 480;//uiHeight;

      pWindowPlugin->m_pWindow = std::move(pWindow);
    }

    // create output target
    {
      ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);

      ezGALSwapChainCreationDescription desc;
      desc.m_pWindow = pWindowPlugin->m_pWindow.Borrow();
      desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
      desc.m_bAllowScreenshots = true;

      pOutput->CreateSwapchain(desc);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // setup render target
    {
      ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
      ezWindowOutputTargetGAL* pOutput = static_cast<ezWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());
      const ezGALSwapChain* pPrimarySwapChain = pDevice->GetSwapChain(pOutput->m_hSwapChain);
      auto hSwapChainRTV = pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture());

      ezGALRenderTargetSetup BackBufferRenderTargetSetup;
      BackBufferRenderTargetSetup.SetRenderTarget(0, hSwapChainRTV);

      const ezSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();
      SetupRenderTarget(BackBufferRenderTargetSetup, wndSize.width, wndSize.height);
    }

    pActor->AddPlugin(std::move(pWindowPlugin));
    ezActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void ezEngineProcessViewContext::SetupRenderTarget(ezGALRenderTargetSetup& renderTargetSetup, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezEngineProcessViewContext::SetupRenderTarget");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetRenderTargetSetup(renderTargetSetup);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void ezEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
  auto pState = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameStateLinkedToWorld(GetDocumentContext()->GetWorld());

  if (pState != nullptr)
  {
    pState->ScheduleRendering();
  }
  // setting to only update one view ?
  // else

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    const ezTag& tagEditor = ezTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

    if (!bRenderEditorGizmos)
    {
      // exclude all editor objects from rendering in proper game views
      pView->m_ExcludeTags.Set(tagEditor);
    }
    else
    {
      pView->m_ExcludeTags.Remove(tagEditor);
    }

    ezRenderWorld::AddMainView(m_hView);
  }
}

bool ezEngineProcessViewContext::FocusCameraOnObject(
  ezCamera& camera, const ezBoundingBoxSphere& objectBounds, float fFov, const ezVec3& vViewDir)
{
  ezVec3 vDir = vViewDir;
  bool bChanged = false;
  ezVec3 vCameraPos = camera.GetCenterPosition();
  ezVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = ezMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / ezMath::Sin(ezAngle::Degree(fFov / 2));
  vDir.Normalize();
  ezVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    camera.LookAt(vNewCameraPos, vCenterPos, ezVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void ezEngineProcessViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezViewRenderMode::Enum renderMode = (ezViewRenderMode::Enum)pMsg->m_uiRenderMode;
  bool bModeFromCamera = false;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView) && pView->GetWorld() != nullptr)
  {
    pView->SetCameraUsageHint(pMsg->m_CameraUsageHint);

    bool bResetDefaultPipeline = true;

    if (const ezCameraComponentManager* pCameraManager = pView->GetWorld()->GetComponentManager<ezCameraComponentManager>())
    {
      // Regardless of the rendering mode, if we have a camera with the same usage hint as the view than use the parameters of that camera.
      if (const ezCameraComponent* pCamera = pCameraManager->GetCameraByUsageHint(pMsg->m_CameraUsageHint))
      {
        bResetDefaultPipeline = !pCamera->GetRenderPipeline().IsValid();
        bModeFromCamera = true;

        pCamera->ApplySettingsToView(pView);
      }
    }

    if (renderMode == ezViewRenderMode::None)
    {
      if (bResetDefaultPipeline)
      {
        pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
      }
    }
    else
    {
      pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
    }
  }

  if (m_Camera.GetCameraMode() != ezCameraMode::Stereo)
  {
    ezCameraMode::Enum cameraMode = (ezCameraMode::Enum)pMsg->m_iCameraMode;
    if (!bModeFromCamera || cameraMode == ezCameraMode::OrthoFixedWidth || cameraMode == ezCameraMode::OrthoFixedHeight)
    {
      m_Camera.SetCameraMode(cameraMode, pMsg->m_fFovOrDim, pMsg->m_fNearPlane, pMsg->m_fFarPlane);
    }

    // prevent too large values
    // sometimes this can happen when imported data is badly scaled and thus way too large
    // then adding dirForwards result in no change and we run into other asserts later
    ezVec3 pos = pMsg->m_vPosition;
    pos.x = ezMath::Clamp(pos.x, -1000000.0f, +1000000.0f);
    pos.y = ezMath::Clamp(pos.y, -1000000.0f, +1000000.0f);
    pos.z = ezMath::Clamp(pos.z, -1000000.0f, +1000000.0f);

    m_Camera.LookAt(pos, pos + pMsg->m_vDirForwards, pMsg->m_vDirUp);
  }

  if (pView)
  {
    pView->SetViewRenderMode(renderMode);

    bool bUseDepthPrePass = renderMode != ezViewRenderMode::WireframeColor && renderMode != ezViewRenderMode::WireframeMonochrome;
    pView->SetRenderPassProperty("DepthPrePass", "Active", bUseDepthPrePass);
    pView->SetRenderPassProperty("AOPass", "Active", bUseDepthPrePass); // Also disable SSAO to save some performance

    // by default this stuff is disabled, derived classes can enable it
    pView->SetRenderPassProperty("EditorSelectionPass", "Active", false);
    pView->SetExtractorProperty("EditorShapeIconsExtractor", "Active", false);
  }
}

ezRenderPipelineResourceHandle ezEngineProcessViewContext::CreateDefaultRenderPipeline()
{
  return ezEditorEngineProcessApp::GetSingleton()->CreateDefaultMainRenderPipeline();
}

ezRenderPipelineResourceHandle ezEngineProcessViewContext::CreateDebugRenderPipeline()
{
  return ezEditorEngineProcessApp::GetSingleton()->CreateDefaultDebugRenderPipeline();
}
