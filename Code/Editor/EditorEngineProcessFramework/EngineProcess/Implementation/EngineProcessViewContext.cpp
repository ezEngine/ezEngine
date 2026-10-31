#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <Core/System/WindowManager.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <Foundation/Containers/StaticArray.h>
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

  ezWindowManager::GetSingleton()->CloseAll(this);
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
    m_bShowOrientationIndicator = pMsg2->m_bShowOrientationIndicator;

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
    auto* pOutputTarget = ezWindowManager::GetSingleton()->GetOutputTarget(m_hEditorWindow);
    if (pOutputTarget->StartCaptureImage().Succeeded())
    {
      m_sPendingScreenshotPath = msg->m_sOutputFile;
    }
  }
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

  auto pWinMan = ezWindowManager::GetSingleton();

  if (!m_hEditorWindow.IsInvalidated())
  {
    // Update window size
    auto* pWindow = static_cast<ezEditorProcessViewWindow*>(pWinMan->GetWindow(m_hEditorWindow));
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

  // create window
  {
    ezUniquePtr<ezEditorProcessViewWindow> pWindow = EZ_DEFAULT_NEW(ezEditorProcessViewWindow);
    if (pWindow->UpdateWindow(hWnd, uiWidth, uiHeight).Failed())
    {
      ezLog::Error("Failed to create Editor Process View Window");
      return;
    }

    // create output target
    ezUniquePtr<ezWindowOutputTargetGAL> pOutput = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL, [this](ezGALSwapChainHandle hSwapChain, ezSizeU32 size)
      { OnSwapChainChanged(hSwapChain, size); });

    ezGALWindowSwapChainCreationDescription desc;
    desc.m_pWindow = pWindow.Borrow();
    desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;

    pOutput->CreateSwapchain(desc);
    if (pOutput->m_hSwapChain.IsInvalidated())
    {
      ezLog::Error("Failed to create swapchain for Editor Process View Window");
      return;
    }

    // setup render target
    {
      const ezSizeU32 wndSize = pWindow->GetClientAreaSize();
      SetupRenderTarget(pOutput->m_hSwapChain, nullptr, static_cast<ezUInt16>(wndSize.width), static_cast<ezUInt16>(wndSize.height));
    }

    // The document guid goes into the name because this process registers one window per open document
    // window, and they are otherwise indistinguishable: anything that picks a window by index or by
    // name (app_info and app_screenshot over MCP) could only ever guess which document it got.
    ezStringBuilder sWindowName("EditorView");

    if (m_pDocumentContext != nullptr)
    {
      ezStringBuilder sGuid;
      ezConversionUtils::ToString(m_pDocumentContext->GetDocumentGuid(), sGuid);
      sWindowName.AppendFormat(" {}", sGuid);
    }

    m_hEditorWindow = pWinMan->Register(sWindowName, this, std::move(pWindow));
    pWinMan->SetOutputTarget(m_hEditorWindow, std::move(pOutput));
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

      if (m_bShowOrientationIndicator)
      {
        DrawOrientationIndicator(pView->GetViewport());
      }
    }

    ezRenderWorld::AddMainView(m_hView);
  }
}

bool ezEngineProcessViewContext::PendingOperationInProgress() const
{
  if (m_sPendingScreenshotPath.IsEmpty())
    return false;

  auto* pOutputTarget = ezWindowManager::GetSingleton()->GetOutputTarget(m_hEditorWindow);
  if (pOutputTarget == nullptr)
  {
    const_cast<ezEngineProcessViewContext*>(this)->m_sPendingScreenshotPath.Clear();
    return false;
  }

  ezImage img;
  ezEnum<ezCaptureImageResult> res = pOutputTarget->WaitCaptureImage(img);

  if (res == ezCaptureImageResult::Pending)
    return true;

  if (res == ezCaptureImageResult::Ready)
  {
    img.SaveTo(m_sPendingScreenshotPath).IgnoreResult();
  }

  const_cast<ezEngineProcessViewContext*>(this)->m_sPendingScreenshotPath.Clear();
  return false;
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
    pView->SetRenderPipelineResource(CreateDefaultEditorRenderPipeline());
    pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("Editor-PostProcessing"), renderMode == ezViewRenderMode::None);
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

    bool bIsWireframe = renderMode == ezViewRenderMode::WireframeColor || renderMode == ezViewRenderMode::WireframeMonochrome;
    if (bIsWireframe)
    {
      pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("Renderer-DepthPrePass"), false);
      pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("Rendering.Pipeline.SSAO"), 0); // Also disable SSAO to save some performance
    }
    else
    {
      // reset to default
      pView->GetBlackboard()->RemoveEntry(ezMakeHashedString("Renderer-DepthPrePass"));
      pView->GetBlackboard()->RemoveEntry(ezMakeHashedString("Rendering.Pipeline.SSAO"));
    }
    SetViewProperties(pView);
  }
}


void ezEngineProcessViewContext::SetViewProperties(ezView* pView)
{
  // by default this stuff is disabled, derived classes can enable it
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("Renderer-ShowSelection1"), false);
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("EditorShapeIconsExtractor.Active"), false);

  // Set clear color of editor views to gray
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("ColorSource0.ClearColor"), ezColorGammaUB(40, 40, 40));
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("ColorSource1.ClearColor"), ezColorGammaUB(40, 40, 40));
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("ColorSource2.ClearColor"), ezColorGammaUB(40, 40, 40));
  pView->GetBlackboard()->SetEntryValue(ezMakeHashedString("ColorSource3.ClearColor"), ezColorGammaUB(40, 40, 40));
}

ezRenderPipelineResourceHandle ezEngineProcessViewContext::CreateDefaultEditorRenderPipeline()
{
  return ezEditorEngineProcessApp::GetSingleton()->CreateDefaultEditorRenderPipeline();
}

ezView* ezEngineProcessViewContext::CreateDefaultView(ezStringView sName)
{
  ezView* pView = nullptr;
  ezRenderWorld::CreateView(sName, pView);
  pView->SetCameraUsageHint(ezCameraUsageHint::EditorView);

  pView->SetBlackboard(ezBlackboard::Create(sName));

  pView->SetRenderPipelineResource(CreateDefaultEditorRenderPipeline());

  ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);

  return pView;
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

void ezEngineProcessViewContext::DrawOrientationIndicator(const ezRectFloat& viewport) const
{
  // all sizes are in pixels, scaled the same way as the debug text (DPI and user preference), so that the labels keep fitting
  const float fScale = ezDebugRenderer::GetTextScale();
  const float fAxisLength = 32.0f * fScale;
  const float fMargin = 16.0f * fScale;
  const float fTipSize = 7.0f * fScale;
  const float fLabelDistance = 11.0f * fScale;
  const float fAxisHalfWidth = 1.5f * fScale;

  if (viewport.width < 4.0f * fAxisLength || viewport.height < 4.0f * fAxisLength)
    return;

  const ezVec2 vCenter(viewport.width - fMargin - fAxisLength, fMargin + fAxisLength);

  const ezVec3 vCamRight = m_Camera.GetCenterDirRight();
  const ezVec3 vCamUp = m_Camera.GetCenterDirUp();
  const ezVec3 vCamForward = m_Camera.GetCenterDirForwards();

  struct Axis
  {
    ezVec2 m_vScreenDir;
    float m_fDepth;
    ezColor m_Color;
    const char* m_szLabel;
  };

  ezStaticArray<Axis, 6> axes;

  auto AddAxis = [&](const ezVec3& vAxis, ezColorScheme::Enum color, const char* szLabel)
  {
    Axis& axis = axes.ExpandAndGetRef();
    axis.m_vScreenDir.Set(vAxis.Dot(vCamRight), -vAxis.Dot(vCamUp)); // screen space Y points down
    axis.m_fDepth = vAxis.Dot(vCamForward);
    axis.m_Color = ezColorScheme::LightUI(color);
    axis.m_szLabel = szLabel;

    // darken axes that point away from the camera, as a depth cue
    axis.m_Color.ScaleRGB(1.0f - 0.45f * ezMath::Max(0.0f, axis.m_fDepth));
  };

  AddAxis(ezVec3(1, 0, 0), ezColorScheme::Red, "X");
  AddAxis(ezVec3(0, 1, 0), ezColorScheme::Green, "Y");
  AddAxis(ezVec3(0, 0, 1), ezColorScheme::Blue, "Z");
  AddAxis(ezVec3(-1, 0, 0), ezColorScheme::Red, nullptr);
  AddAxis(ezVec3(0, -1, 0), ezColorScheme::Green, nullptr);
  AddAxis(ezVec3(0, 0, -1), ezColorScheme::Blue, nullptr);

  // the filled 2D primitives are drawn in order, so start with the ones furthest away
  axes.Sort([](const Axis& a, const Axis& b)
    { return a.m_fDepth > b.m_fDepth; });

  for (const Axis& axis : axes)
  {
    const ezVec2 vTip = vCenter + axis.m_vScreenDir * fAxisLength;
    const ezRectFloat tipRect(vTip.x - fTipSize * 0.5f, vTip.y - fTipSize * 0.5f, fTipSize, fTipSize);

    if (axis.m_szLabel == nullptr)
    {
      // negative axes only get a hollow tip, like in most DCC tools
      ezDebugRenderer::Draw2DLineRectangle(m_hView, tipRect, 0.0f, axis.m_Color);
      continue;
    }

    ezVec2 vDir = axis.m_vScreenDir;
    if (vDir.NormalizeIfNotZero(ezVec2::MakeZero(), 0.05f).Succeeded())
    {
      // the 2D lines are only one pixel wide, so draw the axis as a quad
      const ezVec2 vSide = ezVec2(-vDir.y, vDir.x) * fAxisHalfWidth;
      const ezVec3 v0 = (vCenter - vSide).GetAsVec3(0.0f);
      const ezVec3 v1 = (vCenter + vSide).GetAsVec3(0.0f);
      const ezVec3 v2 = (vTip + vSide).GetAsVec3(0.0f);
      const ezVec3 v3 = (vTip - vSide).GetAsVec3(0.0f);

      const ezDebugRendererTriangle quad[2] = {{v0, v1, v2}, {v0, v2, v3}};
      ezDebugRenderer::Draw2DTriangles(m_hView, quad, axis.m_Color);
    }

    ezDebugRenderer::Draw2DRectangle(m_hView, tipRect, 0.0f, axis.m_Color);

    // if the axis points straight at the camera, vDir is zero and the label ends up on the tip
    const ezVec2 vLabelPos = vTip + vDir * fLabelDistance;
    ezDebugRenderer::Draw2DText(m_hView, axis.m_szLabel, ezVec2I32((ezInt32)vLabelPos.x, (ezInt32)vLabelPos.y), axis.m_Color, 16, ezDebugTextHAlign::Center, ezDebugTextVAlign::Center);
  }
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Win/EngineProcessViewContext_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <EditorEngineProcessFramework/EngineProcess/Implementation/Linux/EngineProcessViewContext_linux.h>
#else
#  error Platform not supported
#endif
