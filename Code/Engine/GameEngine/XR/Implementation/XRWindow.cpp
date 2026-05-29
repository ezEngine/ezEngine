#include <GameEngine/GameEnginePCH.h>

#include "../../../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h"
#include <Core/ResourceManager/ResourceManager.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

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
    m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("XR CompanionView", ezRenderGraphPhase::PostRender);
    ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezWindowOutputTargetXR::OnGALEvent, this));
  }
}

ezWindowOutputTargetXR::~ezWindowOutputTargetXR()
{
  if (m_pCompanionWindowOutputTarget)
  {
    ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezWindowOutputTargetXR::OnGALEvent, this));
  }
  // Delete companion resources.
  ezRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
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

void ezWindowOutputTargetXR::RenderCompanionView()
{
  if (!m_bRender)
    return;

  m_bRender = false;

  EZ_PROFILE_SCOPE("RenderCompanionView");
  ezGALTextureHandle hColorRT = m_pXrInterface->GetCurrentTexture();
  if (hColorRT.IsInvalidated() || !m_pCompanionWindowOutputTarget)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_pCompanionWindowOutputTarget->m_hSwapChain);
  ezGALTextureHandle hCompanionRenderTarget = pSwapChain->GetBackBufferTexture();
  const ezGALTexture* tex = pDevice->GetTexture(hCompanionRenderTarget);
  ezVec2 targetSize = ezVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

  m_pRenderGraph->Reset();

  ezRenderGraphTextureHandle hTarget = m_pRenderGraph->ImportTexture(hCompanionRenderTarget);
  ezRenderGraphTextureHandle hVRSource = m_pRenderGraph->ImportTexture(hColorRT);

  {
    auto pass = m_pRenderGraph->AddGraphicsPass("Blit CompanionView");
    pass.AddColorTarget(hTarget);
    pass.ReadTexture(hVRSource);
    pass.HasSideEffects();
    pass.SetExecuteCallback([this, hVRSource, targetSize](const ezRenderGraphContext& ctx)
      {
      auto* pRenderContext = ctx.GetRenderContext();

      pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      pRenderContext->BindShader(m_hCompanionShader);

      auto* constants = ezRenderContext::GetConstantBufferData<ezVRCompanionViewConstants>(m_hCompanionConstantBuffer);
      constants->TargetSize = targetSize;

      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindBuffer("ezVRCompanionViewConstants", m_hCompanionConstantBuffer);
      bindGroup.BindTexture("VRTexture", ctx.ResolveTexture(hVRSource));

      pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  // If a capture was requested, add a readback pass to the same graph.
  if (m_bCaptureRequested)
  {
    m_bCaptureRequested = false;

    const ezGALTexture* pBackbuffer = pDevice->GetTexture(hCompanionRenderTarget);
    m_CaptureBackbufferDesc = pBackbuffer->GetDescription();

    auto capturePass = m_pRenderGraph->AddTransferPass("CaptureImage");
    capturePass.ReadTexture(hTarget, {}, ezGALResourceState::CopySource);
    capturePass.HasSideEffects();
    capturePass.SetExecuteCallback([this, hTarget](const ezRenderGraphContext& ctx)
      { m_Readback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(hTarget)); });

    m_bCaptureInFlight = true;
  }

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
}

void ezWindowOutputTargetXR::OnGALEvent(const ezGALDeviceEvent& e)
{
  if (e.m_Type != ezGALDeviceEvent::AfterBeginFrame)
    return;

  RenderCompanionView();
}

ezResult ezWindowOutputTargetXR::StartCaptureImage()
{
  if (!m_pCompanionWindowOutputTarget)
    return EZ_FAILURE;

  if (m_bCaptureInFlight || m_bCaptureRequested)
    return EZ_FAILURE;

  m_bCaptureRequested = true;
  return EZ_SUCCESS;
}

ezEnum<ezCaptureImageResult> ezWindowOutputTargetXR::WaitCaptureImage(ezImage& out_image)
{
  if (!m_bCaptureInFlight)
    return ezCaptureImageResult::NotStarted;

  ezEnum<ezGALAsyncResult> res = m_Readback.GetReadbackResult(ezTime::MakeFromHours(1));
  if (res == ezGALAsyncResult::Pending)
    return ezCaptureImageResult::Pending;

  if (res == ezGALAsyncResult::Expired)
  {
    m_bCaptureInFlight = false;
    return ezCaptureImageResult::NotStarted;
  }

  // Ready
  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  ezTempHybridArray<ezGALSystemMemoryDescription, 1> memory;
  ezReadbackTextureLock lock = m_Readback.LockTexture(sourceSubResources, memory);
  if (!lock)
  {
    m_bCaptureInFlight = false;
    return ezCaptureImageResult::NotStarted;
  }

  ezTextureUtils::CopySubResourceToImage(m_CaptureBackbufferDesc, sourceSubResource, memory[0], out_image, true);

  m_bCaptureInFlight = false;
  return ezCaptureImageResult::Ready;
}

const ezWindowOutputTargetBase* ezWindowOutputTargetXR::GetCompanionWindowOutputTarget() const
{
  return m_pCompanionWindowOutputTarget.Borrow();
}


