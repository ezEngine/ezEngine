#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Core/ActorSystem/ActorPluginWindow.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>

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

void ezEngineProcessViewContext::SetViewID(ezUInt32 uiId)
{
  EZ_ASSERT_DEBUG(m_uiViewID == 0xFFFFFFFF, "View ID may only be set once");
  m_uiViewID = uiId;
}

void ezEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);

    SetCamera(pMsg2);

    if (pMsg2->m_uiWindowWidth > 0 && pMsg2->m_uiWindowHeight > 0)
    {
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
      HandleWindowUpdate(reinterpret_cast<ezWindowHandle>(pMsg2->m_uiHWND), pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  else
      ezWindowHandle windowHandle;
      windowHandle.type = ezWindowHandle::Type::XCB;
      windowHandle.xcbWindow.m_Window = static_cast<ezUInt32>(pMsg2->m_uiHWND);
      windowHandle.xcbWindow.m_pConnection = nullptr;
      HandleWindowUpdate(windowHandle, pMsg2->m_uiWindowWidth, pMsg2->m_uiWindowHeight);
#  endif
      Redraw(true);
    }
  }
  else if (const ezViewScreenshotMsgToEngine* msg = ezDynamicCast<const ezViewScreenshotMsgToEngine*>(pMsg))
  {
    ezImage img;
    ezActorPluginWindow* pWindow = m_pEditorWndActor->GetPlugin<ezActorPluginWindow>();
    pWindow->GetOutputTarget()->CaptureImage(img).IgnoreResult();

    img.SaveTo(msg->m_sOutputFile).IgnoreResult();
  }
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  EZ_REPORT_FAILURE("This code path should never be executed on UWP.");
#else
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
    // Update window size
    ezActorPluginWindow* pWindowPlugin = m_pEditorWndActor->GetPlugin<ezActorPluginWindow>();

    auto* pWindow = static_cast<ezEditorProcessViewWindow*>(pWindowPlugin->GetWindow());
    const ezSizeU32 wndSize = pWindow->GetClientAreaSize();

    EZ_ASSERT_DEV(pWindow->GetNativeWindowHandle() == hWnd, "Editor view handle must never change. View needs to be destroyed and recreated.");

    if (wndSize.width == uiWidth && wndSize.height == uiHeight)
      return;

    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      ezLog::Error("Failed to update Editor Process View Window");
    }
    return;
  }

  {
    // Create new actor
    ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "EditorView", this);
    m_pEditorWndActor = pActor.Borrow();

    ezUniquePtr<ezActorPluginWindowOwner> pWindowPlugin = EZ_DEFAULT_NEW(ezActorPluginWindowOwner);

    // create window
    {
      ezUniquePtr<ezEditorProcessViewWindow> pWindow = EZ_DEFAULT_NEW(ezEditorProcessViewWindow);
      if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Succeeded())
      {
        pWindowPlugin->m_pWindow = std::move(pWindow);
      }
      else
      {
        ezLog::Error("Failed to create Editor Process View Window");
      }
    }

    // create output target
    {
      ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL, [this](ezGALSwapChainHandle hSwapChain, ezSizeU32 size)
        { OnSwapChainChanged(hSwapChain, size); });

      ezGALWindowSwapChainCreationDescription desc;
      desc.m_pWindow = pWindowPlugin->m_pWindow.Borrow();
      desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;

      pOutput->CreateSwapchain(desc);

      pWindowPlugin->m_pWindowOutputTarget = std::move(pOutput);
    }

    // setup render target
    {
      ezWindowOutputTargetGAL* pOutput = static_cast<ezWindowOutputTargetGAL*>(pWindowPlugin->m_pWindowOutputTarget.Borrow());

      const ezSizeU32 wndSize = pWindowPlugin->m_pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_hSwapChain, nullptr, static_cast<ezUInt16>(wndSize.width), static_cast<ezUInt16>(wndSize.height));
    }

    pActor->AddPlugin(std::move(pWindowPlugin));
    ezActorManager::GetSingleton()->AddActor(std::move(pActor));
  }
}

void ezEngineProcessViewContext::OnSwapChainChanged(ezGALSwapChainHandle hSwapChain, ezSizeU32 size)
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)size.width, (float)size.height));
    pView->ForceUpdate();
  }
}

void ezEngineProcessViewContext::SetupRenderTarget(ezGALSwapChainHandle hSwapChain, const ezGALRenderTargets* pRenderTargets, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  EZ_LOG_BLOCK("ezEngineProcessViewContext::SetupRenderTarget");
  EZ_ASSERT_DEV((!hSwapChain.IsInvalidated() && pRenderTargets == nullptr) || (hSwapChain.IsInvalidated() && pRenderTargets != nullptr), "hSwapChain and pRenderTargets are mutually exclusive.");

  // setup view
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = CreateView();
    }

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      if (!hSwapChain.IsInvalidated())
        pView->SetSwapChain(hSwapChain);
      else
        pView->SetRenderTargets(*pRenderTargets);

      pView->SetViewport(ezRectFloat(0.0f, 0.0f, (float)uiWidth, (float)uiHeight));
    }
  }
}

void ezEngineProcessViewContext::Redraw(bool bRenderEditorGizmos)
{
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

bool ezEngineProcessViewContext::FocusCameraOnObject(ezCamera& inout_camera, const ezBoundingBoxSphere& objectBounds, float fFov, const ezVec3& vViewDir)
{
  if (!objectBounds.IsValid())
    return false;

  ezVec3 vDir = vViewDir;
  bool bChanged = false;
  ezVec3 vCameraPos = inout_camera.GetCenterPosition();
  ezVec3 vCenterPos = objectBounds.GetSphere().m_vCenter;

  const float fDist = ezMath::Max(0.1f, objectBounds.GetSphere().m_fRadius) / ezMath::Sin(ezAngle::MakeFromDegree(fFov / 2));
  vDir.Normalize();
  ezVec3 vNewCameraPos = vCenterPos - vDir * fDist;
  if (!vNewCameraPos.IsEqual(vCameraPos, 0.01f))
  {
    vCameraPos = vNewCameraPos;
    bChanged = true;
  }

  if (bChanged)
  {
    if (!vNewCameraPos.IsValid())
      return false;

    inout_camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, fFov, 0.1f, 1000.0f);
    inout_camera.LookAt(vNewCameraPos, vCenterPos, ezVec3(0.0f, 0.0f, 1.0f));
  }

  return bChanged;
}

void ezEngineProcessViewContext::SetCamera(const ezViewRedrawMsgToEngine* pMsg)
{
  ezViewRenderMode::Enum renderMode = (ezViewRenderMode::Enum)pMsg->m_uiRenderMode;

  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView) && pView->GetWorld() != nullptr)
  {
    if (renderMode == ezViewRenderMode::None)
    {
      pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
    }
    else
    {
      pView->SetRenderPipelineResource(CreateDebugRenderPipeline());
    }
  }

  if (m_Camera.GetCameraMode() != ezCameraMode::Stereo)
  {
    bool bCameraIsActive = false;
    if (pView && pView->GetWorld())
    {
      ezEnum<ezCameraUsageHint> usageHint = pView->GetCameraUsageHint();
      ezCameraComponent* pComp = pView->GetWorld()->GetOrCreateComponentManager<ezCameraComponentManager>()->GetCameraByUsageHint(usageHint);
      bCameraIsActive = pComp != nullptr && pComp->IsActive();
    }

    // Camera mode should be controlled by a matching camera component if one exists.
    if (!bCameraIsActive)
    {
      ezCameraMode::Enum cameraMode = (ezCameraMode::Enum)pMsg->m_iCameraMode;
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

void ezEngineProcessViewContext::DrawSimpleGrid() const
{
  ezDynamicArray<ezDebugRendererLine> lines;
  lines.Reserve(2 * (10 + 1 + 10) + 4);

  const ezColor xAxisColor = ezColorScheme::LightUI(ezColorScheme::Red) * 0.7f;
  const ezColor yAxisColor = ezColorScheme::LightUI(ezColorScheme::Green) * 0.7f;
  const ezColor gridColor = ezColorScheme::LightUI(ezColorScheme::Gray) * 0.5f;

  // arrows

  const float f = 1.0f;

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, 0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(f, 0.0f, 0.0f);
    l.m_end.Set(f - 0.25f, -0.25f, 0.0f);
    l.m_startColor = xAxisColor;
    l.m_endColor = xAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    auto& l = lines.ExpandAndGetRef();
    l.m_start.Set(0.0f, f, 0.0f);
    l.m_end.Set(-0.25f, f - 0.25f, 0.0f);
    l.m_startColor = yAxisColor;
    l.m_endColor = yAxisColor;
  }

  {
    const float x = 10.0f;

    for (ezInt32 y = -10; y <= +10; ++y)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)-x, (float)y, 0.0f);
      line.m_end.Set((float)+x, (float)y, 0.0f);

      if (y == 0)
      {
        line.m_startColor = xAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  {
    const float y = 10.0f;

    for (ezInt32 x = -10; x <= +10; ++x)
    {
      auto& line = lines.ExpandAndGetRef();

      line.m_start.Set((float)x, (float)-y, 0.0f);
      line.m_end.Set((float)x, (float)+y, 0.0f);

      if (x == 0)
      {
        line.m_startColor = yAxisColor;
      }
      else
      {
        line.m_startColor = gridColor;
      }

      line.m_endColor = line.m_startColor;
    }
  }

  ezDebugRenderer::DrawLines(m_hView, lines, ezColor::White);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Win/EngineProcessViewContext_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
