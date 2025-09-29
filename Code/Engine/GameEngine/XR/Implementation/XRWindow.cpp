#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>

//////////////////////////////////////////////////////////////////////////

ezWindowXR::ezWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> pCompanionWindow)
  : m_pVrInterface(pVrInterface)
  , m_pCompanionWindow(std::move(pCompanionWindow))
{
}

ezWindowXR::~ezWindowXR()
{
  EZ_ASSERT_DEV(m_iReferenceCount == 0, "The window is still being referenced, probably by a swapchain. Make sure to destroy all swapchains and call ezGALDevice::WaitIdle before destroying a window.");
}

ezSizeU32 ezWindowXR::GetClientAreaSize() const
{
  return m_pVrInterface->GetHmdInfo().m_vEyeRenderTargetSize;
}

ezWindowHandle ezWindowXR::GetNativeWindowHandle() const
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->GetNativeWindowHandle();
  }
  return ezWindowHandle();
}

bool ezWindowXR::IsFullscreenWindow(bool bOnlyProperFullscreenMode) const
{
  return true;
}

void ezWindowXR::ProcessWindowMessages()
{
  if (m_pCompanionWindow)
  {
    m_pCompanionWindow->ProcessWindowMessages();
  }
}

const ezWindowBase* ezWindowXR::GetCompanionWindow() const
{
  return m_pCompanionWindow.Borrow();
}

//////////////////////////////////////////////////////////////////////////

ezWindowOutputTargetXR::ezWindowOutputTargetXR(ezXRInterface* pXrInterface, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutputTarget)
  : m_pXrInterface(pXrInterface)
  , m_pCompanionWindowOutputTarget(std::move(pCompanionWindowOutputTarget))
{
  if (m_pCompanionWindowOutputTarget)
  {
    // Create companion resources.
    m_hCompanionShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/VRCompanionView.ezShader");
    EZ_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezVRCompanionViewConstants>();
  }
}

ezWindowOutputTargetXR::~ezWindowOutputTargetXR()
{
  // Delete companion resources.
  ezRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
  m_hCompanionConstantBuffer.Invalidate();
}

void ezWindowOutputTargetXR::PresentImage(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the ezXRInterface now.
}

void ezWindowOutputTargetXR::CompanionViewBeginFrame(bool bThrottleCompanionView)
{
  ezTime currentTime = ezTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + ezTime::MakeFromMilliseconds(16)))
    return;

  m_LastPresent = currentTime;
  ezGALDevice::GetDefaultDevice()->EnqueueFrameSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
  m_bRender = true;
}

void ezWindowOutputTargetXR::CompanionViewEndFrame()
{
  if (!m_bRender)
    return;

  m_bRender = false;

  EZ_PROFILE_SCOPE("RenderCompanionView");
  ezGALTextureHandle m_hColorRT = m_pXrInterface->GetCurrentTexture();
  if (m_hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* m_pRenderContext = ezRenderContext::GetDefaultInstance();

  {
    auto pEncoder = pDevice->BeginCommands("Blit CompanionView");

    const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
    ezGALTextureHandle hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
    const ezGALTexture* tex = pDevice->GetTexture(hCompanionRenderTarget);
    auto hRenderTargetView = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(hCompanionRenderTarget);
    ezVec2 targetSize = ezVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, hRenderTargetView);

    m_pRenderContext->BeginRendering(renderingSetup, ezRectFloat(targetSize.x, targetSize.y));

    m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    m_pRenderContext->BindShader(m_hCompanionShader);

    auto* constants = ezRenderContext::GetConstantBufferData<ezVRCompanionViewConstants>(m_hCompanionConstantBuffer);
    constants->TargetSize = targetSize;

    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroup.BindBuffer("ezVRCompanionViewConstants", m_hCompanionConstantBuffer);
    bindGroup.BindTexture("VRTexture", m_hColorRT);

    m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    m_pRenderContext->EndRendering();

    pDevice->EndCommands(pEncoder);
  }
}

ezResult ezWindowOutputTargetXR::CaptureImage(ezImage& out_image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    // If we are capturing an image, we need to update the companion view first.
    // If not, CompanionViewEndFrame will be called by the XR implementation.
    CompanionViewEndFrame();
    return m_pCompanionWindowOutputTarget->CaptureImage(out_image);
  }
  return EZ_FAILURE;
}

const ezWindowOutputTargetBase* ezWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}


