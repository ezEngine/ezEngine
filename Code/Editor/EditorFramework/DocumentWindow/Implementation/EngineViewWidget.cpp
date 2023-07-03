#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
#  include <Core/System/Window.h>
#  include <RendererCore/RenderContext/RenderContext.h>
#  include <RendererCore/ShaderCompiler/ShaderManager.h>
#  include <RendererFoundation/Device/Device.h>
#  include <RendererFoundation/Device/DeviceFactory.h>
#  include <RendererFoundation/Device/SwapChain.h>
#  include <RendererFoundation/Resources/Texture.h>

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
#    include <xcb/xcb.h>
#  endif
#endif

ezUInt32 ezQtEngineViewWidget::s_uiNextViewID = 0;

ezQtEngineViewWidget::InteractionContext ezQtEngineViewWidget::s_InteractionContext;

void ezObjectPickingResult::Reset()
{
  m_PickedComponent = ezUuid();
  m_PickedObject = ezUuid();
  m_PickedOther = ezUuid();
  m_uiPartIndex = 0;
  m_vPickedPosition.SetZero();
  m_vPickedNormal.SetZero();
  m_vPickingRayStart.SetZero();
}

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
/// \brief Represents the window inside the editor process, into which the engine process renders
class ezEngineViewWindow : public ezWindowBase
{
public:
  ezEngineViewWindow(ezGALDevice* pDevice)
  {
    m_hWnd = INVALID_WINDOW_HANDLE_VALUE;
    m_uiWidth = 0;
    m_uiHeight = 0;
    m_pDevice = pDevice;

    // TODO Resizing
    m_SharedTextureDesc.SetAsRenderTarget(640, 480, ezGALResourceFormat::BGRAUByteNormalizedsRGB);
    for (auto& hSharedTexture : m_hSharedTextures)
    {
      hSharedTexture = pDevice->CreateSharedTexture(m_SharedTextureDesc);
    }

    ezRenderContext::LoadBuiltinShader(ezShaderUtils::ezBuiltinShaderType::CopyImage, m_CopyShader);
  }

  ~ezEngineViewWindow()
  {
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    if (m_hWnd.type == ezWindowHandle::Type::XCB)
    {
      if (!m_hSwapchain.IsInvalidated())
      {
        m_pDevice->DestroySwapChain(m_hSwapchain);
      }
      m_pDevice->WaitIdle();

      EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
      xcb_disconnect(m_hWnd.xcbWindow.m_pConnection);
      m_hWnd.xcbWindow.m_pConnection = nullptr;
      m_hWnd.type = ezWindowHandle::Type::Invalid;
    }
#  endif
  }

  ezResult UpdateWindow(ezWindowHandle hParentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
  {
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    if (m_hWnd.type == ezWindowHandle::Type::Invalid)
    {
      // xcb_connect always returns a non-NULL pointer to a xcb_connection_t,
      // even on failure. Callers need to use xcb_connection_has_error() to
      // check for failure. When finished, use xcb_disconnect() to close the
      // connection and free the structure.
      int scr = 0;
      m_hWnd.type = ezWindowHandle::Type::XCB;
      m_hWnd.xcbWindow.m_pConnection = xcb_connect(NULL, &scr);
      if (auto err = xcb_connection_has_error(m_hWnd.xcbWindow.m_pConnection); err != 0)
      {
        ezLog::Error("Could not connect to x11 via xcb. Error-Code '{}'", err);
        xcb_disconnect(m_hWnd.xcbWindow.m_pConnection);
        m_hWnd.xcbWindow.m_pConnection = nullptr;
        m_hWnd.type = ezWindowHandle::Type::Invalid;
        return EZ_FAILURE;
      }

      m_hWnd.xcbWindow.m_Window = hParentWindow.xcbWindow.m_Window;
#  elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    if (m_hWnd == nullptr)
    {
      m_hWnd = hParentWindow;
#  endif
      // create output target
      {
        ezGALWindowSwapChainCreationDescription desc = {};
        desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
        desc.m_bDoubleBuffered = true;
        desc.m_InitialPresentMode = ezGALPresentMode::VSync;
        desc.m_pWindow = this;

        m_hSwapchain = ezGALWindowSwapChain::Create(desc);
        EZ_ASSERT_ALWAYS(!m_hSwapchain.IsInvalidated(), "Failed to create swapchain"); // TODO better error handling

        ezLog::Info("Creating swapchain at {0}x{1}", uiWidth, uiHeight);
      }
    }
    else if (m_uiWidth != uiWidth || m_uiHeight != uiHeight)
    {
      ezLog::Info("Re-creating the swapchain at {0}x{1}", uiWidth, uiHeight);
      // If the size of the window changed, update the swapchain.
      m_pDevice->UpdateSwapChain(m_hSwapchain, ezGALPresentMode::VSync).AssertSuccess("Failed to resize swapchain");
    }

    m_uiWidth = uiWidth;
    m_uiHeight = uiHeight;
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
    EZ_ASSERT_DEV(hParentWindow.type == ezWindowHandle::Type::XCB && hParentWindow.xcbWindow.m_Window != 0, "Invalid handle passed");
    EZ_ASSERT_DEV(m_hWnd.xcbWindow.m_Window == hParentWindow.xcbWindow.m_Window, "Remote window handle should never change. Window must be destroyed and recreated.");
#  endif
    return EZ_SUCCESS;
  }

  void Render(ezUInt32 uiCurrentTextureIndex, ezUInt64 uiCurrentSemaphoreValue)
  {
    // Begin frame
    m_pDevice->BeginFrame();
    m_pDevice->BeginPipeline("GraphicsTest", m_hSwapchain);

    const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(m_hSharedTextures[uiCurrentTextureIndex]);
    EZ_ASSERT_DEV(pSharedTexture != nullptr, "Shared texture did not resolve");

    pSharedTexture->WaitSemaphoreGPU(uiCurrentSemaphoreValue);

    ezGALPass* pPass = m_pDevice->BeginPass("Clear");

    const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapchain);

    ezGALRenderingSetup setup;

    setup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
    setup.m_ClearColor = ezColor(1.0f, 0.0f, 0.0f, 1.0f);
    setup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    // TODO might not actually need ezRenderContext here
    ezGALRenderCommandEncoder* pEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(pPass, setup, ezRectFloat(static_cast<float>(m_uiWidth), static_cast<float>(m_uiHeight)));

    pEncoder->SetShader(m_CopyShader.m_hActiveGALShader);
    pEncoder->SetBlendState(m_CopyShader.m_hBlendState);
    pEncoder->SetDepthStencilState(m_CopyShader.m_hDepthStencilState);
    pEncoder->SetRasterizerState(m_CopyShader.m_hRasterizerState);

    //pEncoder->SetResourceView(ezGALShaderStage::PixelShader, 0, m_pDevice->GetDefaultResourceView(m_hSharedTextures[uiCurrentTextureIndex]));

    // End Frame
    ezRenderContext::GetDefaultInstance()->EndRendering();
    m_pDevice->EndPass(pPass);
    pPass = nullptr;

    pSharedTexture->SignalSemaphoreGPU(uiCurrentSemaphoreValue + 1);
    m_uiSharedTextureSemaphoreCount[uiCurrentTextureIndex] = uiCurrentSemaphoreValue + 1;

    ezRenderContext::GetDefaultInstance()->ResetContextState();
    m_pDevice->EndPipeline(m_hSwapchain);

    m_pDevice->EndFrame();

    // ezTaskSystem::FinishFrameTasks();
  }

  ezResult FillMessage(ezViewOpenSharedTexturesMsgToEngine& msg)
  {
    msg.m_TextureDesc = m_SharedTextureDesc;
    for (auto& hSharedTexture : m_hSharedTextures)
    {
      const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(hSharedTexture);
      if (pSharedTexture == nullptr)
      {
        return EZ_FAILURE;
      }

      msg.m_TextureHandles.PushBack(pSharedTexture->GetSharedHandle());
    }

    return EZ_SUCCESS;
  }

  void FillMessage(ezViewRedrawMsgToEngine& msg)
  {
    msg.m_uiSharedTextureIndex = m_uiCurrentSharedTextureIndex;
    msg.m_uiSemaphoreCurrentValue = m_uiSharedTextureSemaphoreCount[m_uiCurrentSharedTextureIndex];

    // TODO: Make sure we don't catch our tail
    m_uiCurrentSharedTextureIndex = (m_uiCurrentSharedTextureIndex + 1) % EZ_ARRAY_SIZE(m_uiSharedTextureSemaphoreCount);
  }

  // Inherited via ezWindowBase
  virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(m_uiWidth, m_uiHeight); }
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }
  virtual void ProcessWindowMessages() override {}
  virtual bool IsFullscreenWindow(bool bOnlyProperFullscreenMode = false) const override { return false; }
  virtual void AddReference() override { m_iReferenceCount.Increment(); }
  virtual void RemoveReference() override { m_iReferenceCount.Decrement(); }


  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;

private:
  ezWindowHandle m_hWnd;
  ezAtomicInteger32 m_iReferenceCount = 0;
  ezGALDevice* m_pDevice = nullptr;
  ezGALSwapChainHandle m_hSwapchain;

  ezGALTextureCreationDescription m_SharedTextureDesc;
  ezGALTextureHandle m_hSharedTextures[2];
  ezUInt64 m_uiSharedTextureSemaphoreCount[2] = {0, 0};
  ezUInt32 m_uiCurrentSharedTextureIndex = 0;
  ezShaderUtils::ezBuiltinShader m_CopyShader;
};
#endif

////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget public functions
////////////////////////////////////////////////////////////////////////

ezSizeU32 ezQtEngineViewWidget::s_FixedResolution(0, 0);

ezQtEngineViewWidget::ezQtEngineViewWidget(QWidget* pParent, ezQtEngineDocumentWindow* pDocumentWindow, ezEngineViewConfig* pViewConfig)
  : QWidget(pParent)
  , m_pDocumentWindow(pDocumentWindow)
  , m_pViewConfig(pViewConfig)
{
  // TODO move this somewhere better
#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
  if (!ezGALDevice::HasDefaultDevice())
  {
    const char* szRendererName = "Vulkan";
    const char* szShaderModel = "";
    const char* szShaderCompiler = "";
    ezGALDeviceFactory::GetShaderModelAndCompiler(szRendererName, szShaderModel, szShaderCompiler);

    ezShaderManager::Configure(szShaderModel, true);
    EZ_VERIFY(ezPlugin::LoadPlugin(szShaderCompiler).Succeeded(), "Shader compiler '{}' plugin not found", szShaderCompiler);

    // Create a device
    {
      ezGALDeviceCreationDescription DeviceInit;
      DeviceInit.m_bDebugDevice = true;
      m_pGALDevice = ezGALDeviceFactory::CreateDevice(szRendererName, ezFoundation::GetDefaultAllocator(), DeviceInit);
      if (m_pGALDevice->Init().Failed())
      {
        // TODO better error handling
        EZ_REPORT_FAILURE("Failed to initialize GALDevice");
      }

      ezGALDevice::SetDefaultDevice(m_pGALDevice);
    }
  }
  else
  {
    m_pGALDevice = ezGALDevice::GetDefaultDevice();
  }

  m_pWindow = EZ_DEFAULT_NEW(ezEngineViewWindow, m_pGALDevice);
#endif

  m_pRestartButtonLayout = nullptr;
  m_pRestartButton = nullptr;

  setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  // setAttribute(Qt::WA_OpaquePaintEvent);
  setAutoFillBackground(false);
  setMouseTracking(true);
  setMinimumSize(64, 64); // prevent the window from becoming zero sized, otherwise the rendering code may crash

  setAttribute(Qt::WA_PaintOnScreen, true);
  setAttribute(Qt::WA_NativeWindow, true);
  setAttribute(Qt::WA_NoSystemBackground);

  installEventFilter(this);

  m_bUpdatePickingData = false;
  m_bInDragAndDropOperation = false;

  m_uiViewID = s_uiNextViewID;
  ++s_uiNextViewID;

  m_fCameraLerp = 1.0f;
  m_fCameraTargetFovOrDim = 70.0f;

  ezEditorEngineProcessConnection::s_Events.AddEventHandler(ezMakeDelegate(&ezQtEngineViewWidget::EngineViewProcessEventHandler, this));

  if (ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    ShowRestartButton(true);

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
  ezViewOpenSharedTexturesMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  if (m_pWindow->FillMessage(msg).Succeeded())
  {
    m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
  }
  else
  {
    ezLog::Error("Failed to send initial shared texture create message");
  }
#endif
}


ezQtEngineViewWidget::~ezQtEngineViewWidget()
{
  ezEditorEngineProcessConnection::s_Events.RemoveEventHandler(ezMakeDelegate(&ezQtEngineViewWidget::EngineViewProcessEventHandler, this));

  {
    // Ensure the engine process swap chain is destroyed before the window.
    ezViewDestroyedMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);

    // Wait for engine process response
    auto callback = [&](ezProcessMessage* pMsg) -> bool {
      auto pResponse = static_cast<ezViewDestroyedResponseMsgToEditor*>(pMsg);
      return pResponse->m_DocumentGuid == m_pDocumentWindow->GetDocument()->GetGuid() && pResponse->m_uiViewID == msg.m_uiViewID;
    };
    ezProcessCommunicationChannel::WaitForMessageCallback cb = callback;

    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezGetStaticRTTI<ezViewDestroyedResponseMsgToEditor>(), ezTime::Seconds(5), &cb).Failed())
    {
      ezLog::Error("Timeout while waiting for engine process to destroy view.");
    }
  }

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
  m_pWindow = nullptr;
#endif

  m_pDocumentWindow->RemoveViewWidget(this);
}

void ezQtEngineViewWidget::SyncToEngine()
{
#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  ezWindowHandle hWindow = {};
  hWindow.type = ezWindowHandle::Type::XCB;
  hWindow.xcbWindow.m_Window = winId();
#  elif EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  ezWindowHandle hWindow = {};
  hWindow = reinterpret_cast<ezWindowHandle>(winId());
#  endif

  // TODO what about s_FixedResolution?
  if (m_pWindow->UpdateWindow(hWindow, width() * this->devicePixelRatio(), height() * this->devicePixelRatio()).Failed())
  {
    ezLog::Error("Failed to update window for ezQtEngineViewWidget");
  }
#endif


  // TODO re-enable
  ezViewRedrawMsgToEngine cam;
  cam.m_uiRenderMode = m_pViewConfig->m_RenderMode;

  float fov = m_pViewConfig->m_Camera.GetFovOrDim();
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    ezEditorPreferencesUser* pPref = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();
    fov = pPref->m_fPerspectiveFieldOfView;
  }

  cam.m_uiViewID = GetViewID();
  cam.m_fNearPlane = m_pViewConfig->m_Camera.GetNearPlane();
  cam.m_fFarPlane = m_pViewConfig->m_Camera.GetFarPlane();
  cam.m_iCameraMode = (ezInt8)m_pViewConfig->m_Camera.GetCameraMode();
  cam.m_bUseCameraTransformOnDevice = m_pViewConfig->m_bUseCameraTransformOnDevice;
  cam.m_fFovOrDim = fov;
  cam.m_vDirForwards = m_pViewConfig->m_Camera.GetCenterDirForwards();
  cam.m_vDirUp = m_pViewConfig->m_Camera.GetCenterDirUp();
  cam.m_vDirRight = m_pViewConfig->m_Camera.GetCenterDirRight();
  cam.m_vPosition = m_pViewConfig->m_Camera.GetCenterPosition();
  cam.m_ViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), cam.m_ProjMatrix);

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE
  m_pWindow->FillMessage(cam);
#else
  cam.m_uiHWND = (ezUInt64)(winId());
#endif
  cam.m_uiWindowWidth = width() * this->devicePixelRatio();
  cam.m_uiWindowHeight = height() * this->devicePixelRatio();
  cam.m_bUpdatePickingData = m_bUpdatePickingData;
  cam.m_bEnablePickingSelected = IsPickingAgainstSelectionAllowed() && (!ezEditorInputContext::IsAnyInputContextActive() || ezEditorInputContext::GetActiveInputContext()->IsPickingSelectedAllowed());
  cam.m_bEnablePickTransparent = m_bPickTransparent;

  if (s_FixedResolution.HasNonZeroArea())
  {
    cam.m_uiWindowWidth = s_FixedResolution.width;
    cam.m_uiWindowHeight = s_FixedResolution.height;
  }

  m_pDocumentWindow->GetEditorEngineConnection()->SendMessage(&cam);
}


void ezQtEngineViewWidget::GetCameraMatrices(ezMat4& out_mViewMatrix, ezMat4& out_mProjectionMatrix) const
{
  out_mViewMatrix = m_pViewConfig->m_Camera.GetViewMatrix();
  m_pViewConfig->m_Camera.GetProjectionMatrix((float)width() / (float)height(), out_mProjectionMatrix);
}

void ezQtEngineViewWidget::UpdateCameraInterpolation()
{
  if (m_fCameraLerp >= 1.0f)
    return;

  const ezTime tNow = ezTime::Now();
  const ezTime tDiff = tNow - m_LastCameraUpdate;
  m_LastCameraUpdate = tNow;

  m_fCameraLerp += tDiff.GetSeconds() * 2.0f;

  if (m_fCameraLerp >= 1.0f)
    m_fCameraLerp = 1.0f;

  ezCamera& cam = m_pViewConfig->m_Camera;

  const float fLerpValue = ezMath::Sin(ezAngle::Degree(90.0f * m_fCameraLerp));

  ezQuat qRot, qRotFinal;
  qRot.SetShortestRotation(m_vCameraStartDirection, m_vCameraTargetDirection);
  qRotFinal.SetSlerp(ezQuat::IdentityQuaternion(), qRot, fLerpValue);

  const ezVec3 vNewDirection = qRotFinal * m_vCameraStartDirection;
  const ezVec3 vNewPosition = ezMath::Lerp(m_vCameraStartPosition, m_vCameraTargetPosition, fLerpValue);
  const float fNewFovOrDim = ezMath::Lerp(m_fCameraStartFovOrDim, m_fCameraTargetFovOrDim, fLerpValue);

  /// \todo Hard coded up vector
  cam.LookAt(vNewPosition, vNewPosition + vNewDirection, m_vCameraUp);
  cam.SetCameraMode(cam.GetCameraMode(), fNewFovOrDim, cam.GetNearPlane(), cam.GetFarPlane());
}

void ezQtEngineViewWidget::InterpolateCameraTo(const ezVec3& vPosition, const ezVec3& vDirection, float fFovOrDim, const ezVec3* pNewUpDirection /*= nullptr*/, bool bImmediate /*= false*/)
{
  m_vCameraStartPosition = m_pViewConfig->m_Camera.GetPosition();
  m_vCameraTargetPosition = vPosition;

  m_vCameraStartDirection = m_pViewConfig->m_Camera.GetCenterDirForwards();
  m_vCameraTargetDirection = vDirection;

  if (pNewUpDirection)
    m_vCameraUp = *pNewUpDirection;
  else
    m_vCameraUp = m_pViewConfig->m_Camera.GetCenterDirUp();

  m_vCameraStartDirection.Normalize();
  m_vCameraTargetDirection.Normalize();
  m_vCameraUp.Normalize();


  m_fCameraStartFovOrDim = m_pViewConfig->m_Camera.GetFovOrDim();

  if (fFovOrDim > 0.0f)
    m_fCameraTargetFovOrDim = fFovOrDim;


  EZ_ASSERT_DEV(m_fCameraTargetFovOrDim > 0, "Invalid FOV or ortho dimension");

  if (m_vCameraStartPosition == m_vCameraTargetPosition && m_vCameraStartDirection == m_vCameraTargetDirection && m_fCameraStartFovOrDim == m_fCameraTargetFovOrDim)
    return;

  m_LastCameraUpdate = ezTime::Now();
  m_fCameraLerp = 0.0f;

  if (bImmediate)
  {
    // make sure the next camera update interpolates all the way
    m_LastCameraUpdate -= ezTime::Seconds(10);
    m_fCameraLerp = 0.9f;
  }
}

void ezQtEngineViewWidget::SetEnablePicking(bool bEnable)
{
  m_bUpdatePickingData = bEnable;
}

void ezQtEngineViewWidget::SetPickTransparent(bool bEnable)
{
  if (m_bPickTransparent == bEnable)
    return;

  m_bPickTransparent = bEnable;
  m_LastPickingResult.Reset();
}

void ezQtEngineViewWidget::OpenContextMenu(QPoint globalPos)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  OnOpenContextMenu(globalPos);
}


const ezObjectPickingResult& ezQtEngineViewWidget::PickObject(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY) const
{
  if (!ezEditorEngineProcessConnection::GetSingleton()->IsEngineSetup())
  {
    m_LastPickingResult.Reset();
  }
  else
  {
    ezViewPickingMsgToEngine msg;
    msg.m_uiViewID = GetViewID();
    msg.m_uiPickPosX = uiScreenPosX * devicePixelRatio();
    msg.m_uiPickPosY = uiScreenPosY * devicePixelRatio();

    GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }

  return m_LastPickingResult;
}


ezResult ezQtEngineViewWidget::PickPlane(ezUInt16 uiScreenPosX, ezUInt16 uiScreenPosY, const ezPlane& plane, ezVec3& out_vPosition) const
{
  const auto& cam = m_pViewConfig->m_Camera;

  ezMat4 mView = cam.GetViewMatrix();
  ezMat4 mProj;
  cam.GetProjectionMatrix((float)width() / (float)height(), mProj);
  ezMat4 mViewProj = mProj * mView;
  ezMat4 mInvViewProj = mViewProj.GetInverse();

  ezVec3 vScreenPos(uiScreenPosX, height() - uiScreenPosY, 0);
  ezVec3 vResPos, vResRay;

  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, width(), height(), vScreenPos, vResPos, &vResRay).Failed())
    return EZ_FAILURE;

  if (plane.GetRayIntersection(vResPos, vResRay, nullptr, &out_vPosition))
    return EZ_SUCCESS;

  return EZ_FAILURE;
}

void ezQtEngineViewWidget::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (const ezViewPickingResultMsgToEditor* pFullMsg = ezDynamicCast<const ezViewPickingResultMsgToEditor*>(pMsg))
  {
    m_LastPickingResult.m_PickedObject = pFullMsg->m_ObjectGuid;
    m_LastPickingResult.m_PickedComponent = pFullMsg->m_ComponentGuid;
    m_LastPickingResult.m_PickedOther = pFullMsg->m_OtherGuid;
    m_LastPickingResult.m_uiPartIndex = pFullMsg->m_uiPartIndex;
    m_LastPickingResult.m_vPickedPosition = pFullMsg->m_vPickedPosition;
    m_LastPickingResult.m_vPickedNormal = pFullMsg->m_vPickedNormal;
    m_LastPickingResult.m_vPickingRayStart = pFullMsg->m_vPickingRayStartPosition;

    return;
  }
  else if (const ezViewMarqueePickingResultMsgToEditor* pFullMsg = ezDynamicCast<const ezViewMarqueePickingResultMsgToEditor*>(pMsg))
  {
    HandleMarqueePickingResult(pFullMsg);
    return;
  }
  else if(const ezViewRenderingDoneMsgToEditor* pFullMsg = ezDynamicCast<const ezViewRenderingDoneMsgToEditor*>(pMsg))
  {
    m_pWindow->Render(pFullMsg->m_uiCurrentTextureIndex, pFullMsg->m_uiCurrentSemaphoreValue);
  }
}

ezPlane ezQtEngineViewWidget::GetFallbackPickingPlane(ezVec3 vPointOnPlane) const
{
  if (m_pViewConfig->m_Camera.IsPerspective())
  {
    return ezPlane(ezVec3(0, 0, 1), vPointOnPlane);
  }
  else
  {
    return ezPlane(-m_pViewConfig->m_Camera.GetCenterDirForwards(), vPointOnPlane);
  }
}

void ezQtEngineViewWidget::TakeScreenshot(const char* szOutputPath) const
{
  ezViewScreenshotMsgToEngine msg;
  msg.m_uiViewID = GetViewID();
  msg.m_sOutputFile = szOutputPath;
  m_pDocumentWindow->GetDocument()->SendMessageToEngine(&msg);
}

////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget qt overrides
////////////////////////////////////////////////////////////////////////

bool ezQtEngineViewWidget::eventFilter(QObject* object, QEvent* event)
{
  if (event->type() == QEvent::Type::ShortcutOverride)
  {
    if (ezEditorInputContext::IsAnyInputContextActive())
    {
      // if the active input context does not like other shortcuts,
      // accept this event and thus block further shortcut processing
      // instead Qt will then send a keypress event
      if (ezEditorInputContext::GetActiveInputContext()->GetShortcutsDisabled())
        event->accept();
    }
  }

  return false;
}


void ezQtEngineViewWidget::paintEvent(QPaintEvent* event)
{
  // event->accept();
}

void ezQtEngineViewWidget::resizeEvent(QResizeEvent* event)
{
  m_pDocumentWindow->TriggerRedraw();
  ezLog::Info("QtResizeEvent {0}x{1} to {2}x{3}", event->oldSize().width(), event->oldSize().height(), event->size().width(), event->size().height());
}

void ezQtEngineViewWidget::keyPressEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->KeyPressEvent(e) == ezEditorInput::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyPressEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyPressEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyPressEvent(e);
}

void ezQtEngineViewWidget::keyReleaseEvent(QKeyEvent* e)
{
  if (e->isAutoRepeat())
    return;

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->KeyReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->KeyReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->KeyReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::keyReleaseEvent(e);
}

void ezQtEngineViewWidget::mousePressEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MousePressEvent(e) == ezEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MousePressEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MousePressEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mousePressEvent(e);
}

void ezQtEngineViewWidget::mouseReleaseEvent(QMouseEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MouseReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseReleaseEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseReleaseEvent(e);
}

void ezQtEngineViewWidget::mouseMoveEvent(QMouseEvent* e)
{
  s_InteractionContext.m_pLastHoveredViewWidget = this;
  s_InteractionContext.m_pLastPickingResult = &m_LastPickingResult;

  // kick off the picking
  PickObject(e->pos().x(), e->pos().y());

  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->MouseMoveEvent(e) == ezEditorInput::WasExclusivelyHandled)
    {
      e->accept();
      return;
    }
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    e->accept();
    return;
  }

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->MouseMoveEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->MouseMoveEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
    {
      e->accept();
      return;
    }
  }

  QWidget::mouseMoveEvent(e);
}

void ezQtEngineViewWidget::wheelEvent(QWheelEvent* e)
{
  // if a context is active, it gets exclusive access to the input data
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    if (ezEditorInputContext::GetActiveInputContext()->WheelEvent(e) == ezEditorInput::WasExclusivelyHandled)
      return;
  }

  if (ezEditorInputContext::IsAnyInputContextActive())
    return;

  // Override context
  {
    ezEditorInputContext* pOverride = GetDocumentWindow()->GetDocument()->GetEditorInputContextOverride();
    if (pOverride != nullptr)
    {
      if (pOverride->WheelEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
        return;
    }
  }

  // if no context is active, pass the input through in a certain order, until someone handles it
  for (auto pContext : m_InputContexts)
  {
    if (pContext->WheelEvent(e) == ezEditorInput::WasExclusivelyHandled || ezEditorInputContext::IsAnyInputContextActive())
      return;
  }

  QWidget::wheelEvent(e);
}

void ezQtEngineViewWidget::focusOutEvent(QFocusEvent* e)
{
  if (ezEditorInputContext::IsAnyInputContextActive())
  {
    ezEditorInputContext::GetActiveInputContext()->FocusLost(false);
    ezEditorInputContext::SetActiveInputContext(nullptr);
  }

  QWidget::focusOutEvent(e);
}


void ezQtEngineViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  m_bInDragAndDropOperation = true;
}


void ezQtEngineViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  m_bInDragAndDropOperation = false;
}


void ezQtEngineViewWidget::dropEvent(QDropEvent* e)
{
  m_bInDragAndDropOperation = false;
}


////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget protected functions
////////////////////////////////////////////////////////////////////////

void ezQtEngineViewWidget::EngineViewProcessEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  switch (e.m_Type)
  {
    case ezEditorEngineProcessConnection::Event::Type::ProcessCrashed:
    {
      ShowRestartButton(true);
    }
    break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessStarted:
    {
      ShowRestartButton(false);
    }
    break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessShutdown:
      break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessMessage:
      break;

    case ezEditorEngineProcessConnection::Event::Type::Invalid:
      EZ_ASSERT_DEV(false, "Invalid message should never happen");
      break;

    case ezEditorEngineProcessConnection::Event::Type::ProcessRestarted:
      break;
  }
}

void ezQtEngineViewWidget::ShowRestartButton(bool bShow)
{
  ezQtScopedUpdatesDisabled _(this);

  if (m_pRestartButtonLayout == nullptr && bShow == true)
  {
    m_pRestartButtonLayout = new QHBoxLayout(this);
    m_pRestartButtonLayout->setContentsMargins(0, 0, 0, 0);

    setLayout(m_pRestartButtonLayout);

    m_pRestartButton = new QPushButton(this);
    m_pRestartButton->setText("Restart Engine View Process");
    m_pRestartButton->setVisible(ezEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());
    m_pRestartButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_pRestartButton->connect(m_pRestartButton, &QPushButton::clicked, this, &ezQtEngineViewWidget::SlotRestartEngineProcess);

    m_pRestartButtonLayout->addWidget(m_pRestartButton);
  }

  if (m_pRestartButton)
  {
    m_pRestartButton->setVisible(bShow);

    if (bShow)
      m_pRestartButton->update();
  }
}


////////////////////////////////////////////////////////////////////////
// ezQtEngineViewWidget private slots
////////////////////////////////////////////////////////////////////////

void ezQtEngineViewWidget::SlotRestartEngineProcess()
{
  ezEditorEngineProcessConnection::GetSingleton()->RestartProcess().IgnoreResult();
}


////////////////////////////////////////////////////////////////////////
// ezQtViewWidgetContainer
////////////////////////////////////////////////////////////////////////

ezQtViewWidgetContainer::ezQtViewWidgetContainer(QWidget* pParent, ezQtEngineViewWidget* pViewWidget, const char* szToolBarMapping)
  : QWidget(pParent)
{
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);

  m_pLayout = new QVBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pViewWidget = pViewWidget;
  m_pViewWidget->setParent(this);

  if (!ezStringUtils::IsNullOrEmpty(szToolBarMapping))
  {
    // Add Tool Bar
    ezQtToolBarActionMapView* pToolBar = new ezQtToolBarActionMapView("Toolbar", this);
    ezActionContext context;
    context.m_sMapping = szToolBarMapping;
    context.m_pDocument = pViewWidget->GetDocumentWindow()->GetDocument();
    context.m_pWindow = m_pViewWidget;
    pToolBar->SetActionContext(context);
    m_pLayout->addWidget(pToolBar, 0);
  }

  m_pLayout->addWidget(m_pViewWidget, 1);
}

ezQtViewWidgetContainer::~ezQtViewWidgetContainer() = default;
