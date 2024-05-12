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

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorPluginWindowXR, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


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

void ezWindowOutputTargetXR::Present(bool bEnableVSync)
{
  // Swapchain present is handled by the rendering of the view automatically and RenderCompanionView is called by the ezXRInterface now.
}

void ezWindowOutputTargetXR::RenderCompanionView(bool bThrottleCompanionView)
{
  ezTime currentTime = ezTime::Now();
  if (bThrottleCompanionView && currentTime < (m_LastPresent + ezTime::MakeFromMilliseconds(16)))
    return;

  m_LastPresent = currentTime;

  EZ_PROFILE_SCOPE("RenderCompanionView");
  ezGALTextureHandle m_hColorRT = m_pXrInterface->GetCurrentTexture();
  if (m_hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* m_pRenderContext = ezRenderContext::GetDefaultInstance();

  {
    pDevice->BeginPipeline("VR CompanionView", m_pCompanionWindowOutputTarget->m_hSwapChain);

    auto pPass = pDevice->BeginPass("Blit CompanionView");

    const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
    ezGALTextureHandle hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
    const ezGALTexture* tex = pDevice->GetTexture(hCompanionRenderTarget);
    auto hRenderTargetView = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(hCompanionRenderTarget);
    ezVec2 targetSize = ezVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hRenderTargetView);

    m_pRenderContext->BeginRendering(pPass, renderingSetup, ezRectFloat(targetSize.x, targetSize.y));

    m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    m_pRenderContext->BindConstantBuffer("ezVRCompanionViewConstants", m_hCompanionConstantBuffer);
    m_pRenderContext->BindShader(m_hCompanionShader);

    auto* constants = ezRenderContext::GetConstantBufferData<ezVRCompanionViewConstants>(m_hCompanionConstantBuffer);
    constants->TargetSize = targetSize;

    ezGALTextureResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
    m_pRenderContext->BindTexture2D("VRTexture", hInputView);
    m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    m_pRenderContext->EndRendering();

    pDevice->EndPass(pPass);

    pDevice->EndPipeline(m_pCompanionWindowOutputTarget->m_hSwapChain);
    m_pRenderContext->ResetContextState();
  }
}

ezResult ezWindowOutputTargetXR::CaptureImage(ezImage& out_image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    return m_pCompanionWindowOutputTarget->CaptureImage(out_image);
  }
  return EZ_FAILURE;
}

const ezWindowOutputTargetBase* ezWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

ezActorPluginWindowXR::ezActorPluginWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> pCompanionWindow, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutput)
  : m_pVrInterface(pVrInterface)
{
  m_pWindow = EZ_DEFAULT_NEW(ezWindowXR, pVrInterface, std::move(pCompanionWindow));
  m_pWindowOutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetXR, pVrInterface, std::move(pCompanionWindowOutput));
}

ezActorPluginWindowXR::~ezActorPluginWindowXR()
{
  m_pVrInterface->OnActorDestroyed();
}

void ezActorPluginWindowXR::Initialize() {}

ezWindowBase* ezActorPluginWindowXR::GetWindow() const
{
  return m_pWindow.Borrow();
}

ezWindowOutputTargetBase* ezActorPluginWindowXR::GetOutputTarget() const
{
  return m_pWindowOutputTarget.Borrow();
}

void ezActorPluginWindowXR::Update()
{
  if (GetWindow())
  {
    GetWindow()->ProcessWindowMessages();
  }
}

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRWindow);
