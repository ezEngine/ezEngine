#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Renderer/EngineViewWindow.h>
#include <EditorFramework/Renderer/EditorRendererSubsystem.h>

#ifdef BUILDSYSTEM_ENGINE_PROCESS_SHARED_TEXTURE

#  include <Foundation/Strings/FormatString.h>
#  include <Foundation/Time/Time.h>
#  include <Core/ResourceManager/ResourceManager.h>
#  include <RendererCore/RenderContext/RenderContext.h>
#  include <RendererFoundation/Resources/Texture.h>
#  include <RendererCore/Textures/TextureUtils.h>
#  include <Texture/Image/Image.h>

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
#    include <xcb/xcb.h>
#  endif

namespace
{
  static const ezTime s_InFlightTimeout = ezTime::MakeFromSeconds(5.0);
}

ezEngineViewWindow::ezEngineViewWindow(ezGALDevice* pDevice)
  : m_pDevice(pDevice)
{
  m_hWnd = INVALID_WINDOW_HANDLE_VALUE;
  m_hCopyShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Copy.ezShader");
  EZ_ASSERT_DEV(m_hCopyShader.IsValid(), "Failed to load copy shader for editor view window.");
  ezEditorRendererSubsystem::RegisterViewWindow(this);
}

ezEngineViewWindow::~ezEngineViewWindow()
{
  ezEditorRendererSubsystem::UnregisterViewWindow(this);
  if (m_pDevice)
  {
    m_hCopyShader.Invalidate();
    if (!m_hSwapChain.IsInvalidated())
    {
      m_pDevice->DestroySwapChain(m_hSwapChain);
      m_hSwapChain.Invalidate();
    }

    DestroySharedTextures();
    m_pDevice->WaitIdle();
  }

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (m_hWnd.type == ezWindowHandle::Type::XCB && m_hWnd.xcbWindow.m_pConnection != nullptr)
  {
    EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
    xcb_disconnect(m_hWnd.xcbWindow.m_pConnection);
    m_hWnd.xcbWindow.m_pConnection = nullptr;
    m_hWnd.type = ezWindowHandle::Type::Invalid;
  }
#  endif
}

ezResult ezEngineViewWindow::UpdateWindow(ezWindowHandle hParentWindow, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  if (m_bResizePending)
  {
    ClearTimedOutInFlightTextures(ezTime::Now());
    if (!HasInFlightTextures())
    {
      uiWidth = m_uiPendingWidth;
      uiHeight = m_uiPendingHeight;
      m_bResizePending = false;
    }
  }
  if (m_bReopenSharedTextures)
  {
    ezUInt16 uiReopenWidth = uiWidth;
    ezUInt16 uiReopenHeight = uiHeight;
    if (uiReopenWidth == 0 || uiReopenHeight == 0)
    {
      uiReopenWidth = m_uiWidth;
      uiReopenHeight = m_uiHeight;
    }
    if (uiReopenWidth > 0 && uiReopenHeight > 0)
    {
      CreateSharedTextures(uiReopenWidth, uiReopenHeight);
    }
    m_bReopenSharedTextures = false;
  }
#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  if (m_hWnd.type == ezWindowHandle::Type::Invalid)
  {
    int scr = 0;
    m_hWnd.type = ezWindowHandle::Type::XCB;
    m_hWnd.xcbWindow.m_pConnection = xcb_connect(nullptr, &scr);
    if (auto err = xcb_connection_has_error(m_hWnd.xcbWindow.m_pConnection); err != 0)
    {
      ezLog::Error("Could not connect to X11 via xcb. Error code '{}'", err);
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
    ezGALWindowSwapChainCreationDescription desc;
    desc.m_BackBufferFormat = ezGALResourceFormat::BGRAUByteNormalizedsRGB;
    desc.m_bDoubleBuffered = true;
    desc.m_InitialPresentMode = ezGALPresentMode::VSync;
    desc.m_pWindow = this;

    m_hSwapChain = ezGALWindowSwapChain::Create(desc);
    EZ_ASSERT_ALWAYS(!m_hSwapChain.IsInvalidated(), "Failed to create swapchain");
    CreateSharedTextures(uiWidth, uiHeight);
  }
  else if (m_uiWidth != uiWidth || m_uiHeight != uiHeight)
  {
    ClearTimedOutInFlightTextures(ezTime::Now());
    if (HasInFlightTextures())
    {
      m_bResizePending = true;
      m_uiPendingWidth = uiWidth;
      m_uiPendingHeight = uiHeight;
      return EZ_SUCCESS;
    }

    CreateSharedTextures(uiWidth, uiHeight);
    m_bSwapChainResizePending = true;
  }

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

#  if EZ_ENABLED(EZ_PLATFORM_LINUX)
  EZ_ASSERT_DEV(hParentWindow.type == ezWindowHandle::Type::XCB && hParentWindow.xcbWindow.m_Window != 0, "Invalid handle passed");
  EZ_ASSERT_DEV(m_hWnd.xcbWindow.m_Window == hParentWindow.xcbWindow.m_Window, "Remote window handle should never change. Window must be destroyed and recreated.");
#  endif

  return EZ_SUCCESS;
}

void ezEngineViewWindow::RequestScreenshot(ezStringView sOutputFile)
{
  m_sPendingScreenshotFile = sOutputFile;
}

ezResult ezEngineViewWindow::FillOpenMessage(ezViewOpenSharedTexturesMsgToEngine& msg)
{
  if (!m_bOpenMessagePending)
  {
    return EZ_FAILURE;
  }

  msg.m_TextureDesc = m_SharedTextureDesc;
  msg.m_TextureHandles.Clear();
  for (auto& hSharedTexture : m_hSharedTextures)
  {
    const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(hSharedTexture);
    if (pSharedTexture == nullptr)
    {
      return EZ_FAILURE;
    }

    msg.m_TextureHandles.PushBack(pSharedTexture->GetSharedHandle());
  }

  m_bOpenMessagePending = false;
  ezLog::Warning("[SharedTex] Editor SEND open message: gen={}, textureCount={}", m_uiTextureGeneration, msg.m_TextureHandles.GetCount());
  return EZ_SUCCESS;
}

ezResult ezEngineViewWindow::FillRedrawMessage(ezViewRedrawMsgToEngine& msg)
{
  if (m_bOpenMessagePending)
  {
    return EZ_FAILURE;
  }

  const ezTime now = ezTime::Now();
  ClearTimedOutInFlightTextures(now);

  ezUInt32 uiIndex = m_uiCurrentSharedTextureIndex;
  for (ezUInt32 i = 0; i < s_SharedTextureCount; ++i)
  {
    if (!m_bSharedTextureInUse[uiIndex])
    {
      msg.m_uiSharedTextureIndex = uiIndex;
      msg.m_uiSemaphoreCurrentValue = m_uiSharedTextureSemaphoreCount[uiIndex];
      m_bSharedTextureInUse[uiIndex] = true;
      m_bInFlightTimedOut[uiIndex] = false;
      m_TextureInFlightStart[uiIndex] = now;
      m_uiCurrentSharedTextureIndex = (uiIndex + 1) % s_SharedTextureCount;
      ezLog::Warning("[SharedTex] Editor SEND redraw: gen={}, tex={}, sem={}", m_uiTextureGeneration, uiIndex, m_uiSharedTextureSemaphoreCount[uiIndex]);
      return EZ_SUCCESS;
    }
    uiIndex = (uiIndex + 1) % s_SharedTextureCount;
  }

  return EZ_FAILURE;
}

void ezEngineViewWindow::TryCompleteScreenshotReadback()
{
  if (!m_bReadbackInFlight)
  {
    return;
  }

  if (m_Readback.GetReadbackResult(ezTime::MakeFromSeconds(5)) != ezGALAsyncResult::Ready)
  {
    ezLog::Warning("Timeout while waiting for shared texture readback");
    return;
  }

  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<const ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  ezDynamicArray<ezGALSystemMemoryDescription> memory;
  ezReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
  if (!lock)
  {
    ezLog::Warning("Failed to lock shared texture readback");
    return;
  }

  const ezGALTexture* pTexture = m_pDevice->GetTexture(m_hSharedTextures[m_uiReadbackTextureIndex]);
  if (pTexture == nullptr)
  {
    return;
  }

  ezImage image;
  ezTextureUtils::CopySubResourceToImage(pTexture->GetDescription(), sourceSubResource, memory[0], image, true);
  image.SaveTo(m_sPendingScreenshotFile).IgnoreResult();
  m_sPendingScreenshotFile.Clear();
  m_bReadbackInFlight = false;
}

void ezEngineViewWindow::Render(ezUInt32 uiCurrentTextureIndex, ezUInt64 uiCurrentSemaphoreValue)
{
  if (uiCurrentTextureIndex >= s_SharedTextureCount)
  {
    return;
  }

  ezLog::Warning("[SharedTex] Editor RECV render done: gen={}, tex={}, sem={}, inUse={}, localSem={}", m_uiTextureGeneration, uiCurrentTextureIndex, uiCurrentSemaphoreValue, m_bSharedTextureInUse[uiCurrentTextureIndex], m_uiSharedTextureSemaphoreCount[uiCurrentTextureIndex]);

  // Reject stale callbacks from before a shared texture recreation. After CreateSharedTextures
  // resets all in-use flags, any late engine responses referencing the old textures must be
  // discarded. Without this, the stale callback would read uninitialized content from the new
  // texture and poison the semaphore counter, permanently breaking that texture slot.
  if (!m_bSharedTextureInUse[uiCurrentTextureIndex])
  {
    ezLog::Warning("[SharedTex] Editor REJECTED stale: gen={}, tex={}, sem={}", m_uiTextureGeneration, uiCurrentTextureIndex, uiCurrentSemaphoreValue);
    return;
  }

  if (m_uiSharedTextureSemaphoreCount[uiCurrentTextureIndex] >= uiCurrentSemaphoreValue)
  {
    ezLog::Warning("[SharedTex] Editor REJECTED outdated semaphore: gen={}, tex={}, sem={}, localSem={}", m_uiTextureGeneration, uiCurrentTextureIndex, uiCurrentSemaphoreValue, m_uiSharedTextureSemaphoreCount[uiCurrentTextureIndex]);
    ResetInFlightTexture(uiCurrentTextureIndex);
    return;
  }

  if (m_hSwapChain.IsInvalidated())
  {
    ResetInFlightTexture(uiCurrentTextureIndex);
    return;
  }

  const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(m_hSharedTextures[uiCurrentTextureIndex]);
  if (pSharedTexture == nullptr)
  {
    ResetInFlightTexture(uiCurrentTextureIndex);
    return;
  }

  if (m_bSwapChainResizePending)
  {
    m_pDevice->BeginFrame();
    m_pDevice->EndFrame();
    m_pDevice->WaitIdle();
    m_pDevice->UpdateSwapChain(m_hSwapChain, ezGALPresentMode::VSync).IgnoreResult();
    m_bSwapChainResizePending = false;
  }

  m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
  m_pDevice->BeginFrame();
  {
    ezGALCommandEncoder* pCommandEncoder = m_pDevice->BeginCommands("EditorViewSharedTextureCopy");
    EZ_SCOPE_EXIT(m_pDevice->EndCommands(pCommandEncoder));

    pSharedTexture->WaitSemaphoreGPU(uiCurrentSemaphoreValue);

    if (!m_sPendingScreenshotFile.IsEmpty() && !m_bReadbackInFlight)
    {
      m_Readback.ReadbackTexture(*pCommandEncoder, m_hSharedTextures[uiCurrentTextureIndex]);
      m_bReadbackInFlight = true;
      m_uiReadbackTextureIndex = uiCurrentTextureIndex;
    }

    const ezGALTextureHandle hBackBuffer = m_pDevice->GetBackBufferTextureFromSwapChain(m_hSwapChain);
    if (!hBackBuffer.IsInvalidated())
    {
      const ezGALTexture* pBackBufferTexture = m_pDevice->GetTexture(hBackBuffer);
      if (pBackBufferTexture != nullptr)
      {
        ezGALRenderTargetViewHandle hBackbufferRTV = m_pDevice->GetDefaultRenderTargetView(hBackBuffer);
        ezGALRenderingSetup renderingSetup;
        renderingSetup.SetColorTarget(0, hBackbufferRTV);

        const auto& backBufferDesc = pBackBufferTexture->GetDescription();
        ezRectFloat viewport(static_cast<float>(backBufferDesc.m_uiWidth), static_cast<float>(backBufferDesc.m_uiHeight));

        ezRenderContext* pRenderContext = ezRenderContext::GetDefaultInstance();
        pRenderContext->BeginRendering(renderingSetup, viewport, "EditorViewSharedTextureCopy");
        pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
        pRenderContext->BindShader(m_hCopyShader);

        ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
        bindGroup.BindTexture("Input", m_hSharedTextures[uiCurrentTextureIndex]);

        pRenderContext->DrawMeshBuffer().AssertSuccess();
        pRenderContext->EndRendering();
      }
    }

    pSharedTexture->SignalSemaphoreGPU(uiCurrentSemaphoreValue + 1);
  }
  m_pDevice->EndFrame();

  if (m_bReadbackInFlight)
  {
    TryCompleteScreenshotReadback();
  }

  m_uiSharedTextureSemaphoreCount[uiCurrentTextureIndex] = uiCurrentSemaphoreValue + 1;
  ResetInFlightTexture(uiCurrentTextureIndex);
}

ezSizeU32 ezEngineViewWindow::GetClientAreaSize() const
{
  return ezSizeU32(m_uiWidth, m_uiHeight);
}

ezWindowHandle ezEngineViewWindow::GetNativeWindowHandle() const
{
  return m_hWnd;
}

void ezEngineViewWindow::ProcessWindowMessages()
{
}

bool ezEngineViewWindow::IsFullscreenWindow(bool bOnlyProperFullscreenMode) const
{
  return false;
}

bool ezEngineViewWindow::IsVisible() const
{
  return true;
}

void ezEngineViewWindow::AddReference()
{
  m_iReferenceCount.Increment();
}

void ezEngineViewWindow::RemoveReference()
{
  m_iReferenceCount.Decrement();
}

void ezEngineViewWindow::CreateSharedTextures(ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  DestroySharedTextures();

  m_SharedTextureDesc.SetAsRenderTarget(uiWidth, uiHeight, ezGALResourceFormat::BGRAUByteNormalizedsRGB);
  m_SharedTextureDesc.m_Type = ezGALTextureType::Texture2DShared;

  for (ezUInt32 i = 0; i < s_SharedTextureCount; ++i)
  {
    m_hSharedTextures[i] = m_pDevice->CreateSharedTexture(m_SharedTextureDesc);
    EZ_ASSERT_DEV(!m_hSharedTextures[i].IsInvalidated(), "Failed to create shared texture for editor view");
    m_uiSharedTextureSemaphoreCount[i] = 0;
    m_bSharedTextureInUse[i] = false;
    m_bInFlightTimedOut[i] = false;
    m_TextureInFlightStart[i] = ezTime::MakeZero();
  }

  m_uiCurrentSharedTextureIndex = 0;
  m_uiTextureGeneration++;
  m_bOpenMessagePending = true;
  ezLog::Warning("[SharedTex] Editor CREATE textures: gen={}, size={}x{}", m_uiTextureGeneration, uiWidth, uiHeight);
}

void ezEngineViewWindow::DestroySharedTextures()
{
  bool bAnyTextureValid = false;
  for (const auto& hSharedTexture : m_hSharedTextures)
  {
    bAnyTextureValid |= !hSharedTexture.IsInvalidated();
  }

  if (bAnyTextureValid)
  {
    m_pDevice->WaitIdle();
  }

  for (auto& hSharedTexture : m_hSharedTextures)
  {
    if (!hSharedTexture.IsInvalidated())
    {
      m_pDevice->DestroySharedTexture(hSharedTexture);
      hSharedTexture.Invalidate();
    }
  }

  for (ezUInt32 i = 0; i < s_SharedTextureCount; ++i)
  {
    m_uiSharedTextureSemaphoreCount[i] = 0;
    m_bSharedTextureInUse[i] = false;
    m_bInFlightTimedOut[i] = false;
    m_TextureInFlightStart[i] = ezTime::MakeZero();
  }
}

bool ezEngineViewWindow::HasInFlightTextures() const
{
  for (ezUInt32 i = 0; i < s_SharedTextureCount; ++i)
  {
    if (m_bSharedTextureInUse[i])
    {
      return true;
    }
  }

  return false;
}

void ezEngineViewWindow::ClearTimedOutInFlightTextures(const ezTime& now)
{
  for (ezUInt32 i = 0; i < s_SharedTextureCount; ++i)
  {
    if (!m_bSharedTextureInUse[i])
    {
      continue;
    }

    if ((now - m_TextureInFlightStart[i]) <= s_InFlightTimeout)
    {
      continue;
    }

    if (!m_bInFlightTimedOut[i])
    {
      ezLog::Warning("Shared texture {} was in flight for {} seconds without a response. Releasing it.", i, ezArgF((now - m_TextureInFlightStart[i]).GetSeconds(), 2));
      m_bInFlightTimedOut[i] = true;
    }
    m_bReopenSharedTextures = true;
    ResetInFlightTexture(i);
  }
}

void ezEngineViewWindow::ResetInFlightTexture(ezUInt32 uiIndex)
{
  m_bSharedTextureInUse[uiIndex] = false;
  m_TextureInFlightStart[uiIndex] = ezTime::MakeZero();
}

#endif
