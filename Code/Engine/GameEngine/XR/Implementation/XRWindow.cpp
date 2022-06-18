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

ezWindowXR::~ezWindowXR() {}

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

    // Companion window output is assumed to be ezWindowOutputTargetGAL
    ezWindowOutputTargetGAL* pOutputGAL = static_cast<ezWindowOutputTargetGAL*>(m_pCompanionWindowOutputTarget.Borrow());
    const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(pOutputGAL->m_hSwapChain);
    m_hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
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
  if (bThrottleCompanionView && currentTime < (m_lastPresent + ezTime::Milliseconds(16)))
    return;

  m_lastPresent = currentTime;

  EZ_PROFILE_SCOPE("RenderCompanionView");
  ezGALTextureHandle m_hColorRT = m_pXrInterface->GetCurrentTexture();
  if (m_hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* m_pRenderContext = ezRenderContext::GetDefaultInstance();

  if (const ezGALTexture* tex = pDevice->GetTexture(m_hCompanionRenderTarget))
  {
    pDevice->BeginPipeline("VR CompanionView", m_pCompanionWindowOutputTarget->m_hSwapChain);

    auto pPass = pDevice->BeginPass("Blit CompanionView");

    auto hRenderTargetView = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hCompanionRenderTarget);
    ezVec2 targetSize = ezVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, hRenderTargetView);

    m_pRenderContext->BeginRendering(pPass, renderingSetup, ezRectFloat(targetSize.x, targetSize.y));

    m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    m_pRenderContext->BindConstantBuffer("ezVRCompanionViewConstants", m_hCompanionConstantBuffer);
    m_pRenderContext->BindShader(m_hCompanionShader);

    auto* constants = ezRenderContext::GetConstantBufferData<ezVRCompanionViewConstants>(m_hCompanionConstantBuffer);
    constants->TargetSize = targetSize;

    ezGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
    m_pRenderContext->BindTexture2D("VRTexture", hInputView);
    m_pRenderContext->DrawMeshBuffer().IgnoreResult();

    m_pRenderContext->EndRendering();

    pDevice->EndPass(pPass);

    pDevice->EndPipeline(m_pCompanionWindowOutputTarget->m_hSwapChain);
    m_pRenderContext->ResetContextState();
  }
}

ezResult ezWindowOutputTargetXR::CaptureImage(ezImage& out_Image)
{
  if (m_pCompanionWindowOutputTarget)
  {
    return m_pCompanionWindowOutputTarget->CaptureImage(out_Image);
  }
  return EZ_FAILURE;
}

const ezWindowOutputTargetBase* ezWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}

//////////////////////////////////////////////////////////////////////////

ezActorPluginWindowXR::ezActorPluginWindowXR(ezXRInterface* pVrInterface, ezUniquePtr<ezWindowBase> companionWindow, ezUniquePtr<ezWindowOutputTargetGAL> companionWindowOutput)
  : m_pVrInterface(pVrInterface)
{
  m_pWindow = EZ_DEFAULT_NEW(ezWindowXR, pVrInterface, std::move(companionWindow));
  m_pWindowOutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetXR, pVrInterface, std::move(companionWindowOutput));
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
