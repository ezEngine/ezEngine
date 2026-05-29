#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Logging/Log.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphManager.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

ezWindowOutputTargetGAL::ezWindowOutputTargetGAL(OnSwapChainChanged onSwapChainChanged)
  : m_OnSwapChainChanged(onSwapChainChanged)
{
  ezGALDevice::GetDefaultDevice()->s_SwapChainUpdatedEvent.AddEventHandler(ezMakeDelegate(&ezWindowOutputTargetGAL::SwapChainUpdatedEventHandler, this));
  ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezWindowOutputTargetGAL::OnRenderEvent, this));
  m_pRenderGraph = ezRenderGraphManager::CreateRenderGraph("CaptureImage", ezRenderGraphPhase::PostRender);
}

ezWindowOutputTargetGAL::~ezWindowOutputTargetGAL()
{
  ezGALDevice::GetDefaultDevice()->s_SwapChainUpdatedEvent.RemoveEventHandler(ezMakeDelegate(&ezWindowOutputTargetGAL::SwapChainUpdatedEventHandler, this));
  ezGALDevice::s_Events.RemoveEventHandler(ezMakeDelegate(&ezWindowOutputTargetGAL::OnRenderEvent, this));

  ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  ezGALDevice::GetDefaultDevice()->WaitIdle();
}

void ezWindowOutputTargetGAL::CreateSwapchain(const ezGALWindowSwapChainCreationDescription& desc)
{
  m_CurrentDesc = desc;
  // ezWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_CurrentDesc.m_InitialPresentMode = ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate;
  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    pDevice->UpdateSwapChain(m_hSwapChain, ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate).AssertSuccess("");
  }
  else
  {
    m_Size = desc.m_pWindow->GetClientAreaSize();
    m_hSwapChain = ezGALWindowSwapChain::Create(m_CurrentDesc);
  }
}

void ezWindowOutputTargetGAL::PresentImage(bool bEnableVSync)
{
  // For now, the actual present call is done during ezGALDevice::EndFrame by calling ezGALDevice::EnqueueFrameSwapChain before the render loop.
}

void ezWindowOutputTargetGAL::AcquireImage()
{
  // For now, the actual acquire call is done during ezGALDevice::BeginFrame by calling ezGALDevice::EnqueueFrameSwapChain before the render loop.
  // This call is only used to recreate the swapchain at a safe location.

  ezEnum<ezGALPresentMode> presentMode = ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate;

  if (m_Size != m_CurrentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_CurrentDesc.m_InitialPresentMode)
  {
    CreateSwapchain(m_CurrentDesc);
  }
}

ezResult ezWindowOutputTargetGAL::StartCaptureImage()
{
  if (m_bCaptureInFlight || m_bCaptureRequested)
    return EZ_FAILURE;

  m_bCaptureRequested = true;
  return EZ_SUCCESS;
}

void ezWindowOutputTargetGAL::OnRenderEvent(const ezGALDeviceEvent& e)
{
  if (e.m_Type != ezGALDeviceEvent::AfterBeginFrame)
    return;

  if (!m_bCaptureRequested)
    return;

  m_bCaptureRequested = false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  ezGALTextureHandle hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : ezGALTextureHandle();
  if (hBackbuffer.IsInvalidated())
    return;

  const ezGALTexture* pBackbuffer = pDevice->GetTexture(hBackbuffer);
  m_CaptureBackbufferDesc = pBackbuffer->GetDescription();

  m_pRenderGraph->Reset();

  ezRenderGraphTextureHandle hTex = m_pRenderGraph->ImportTexture(hBackbuffer);

  auto pass = m_pRenderGraph->AddTransferPass("CaptureImage");
  pass.ReadTexture(hTex, {}, ezGALResourceState::CopySource);
  pass.HasSideEffects();
  pass.SetExecuteCallback([this, hTex](const ezRenderGraphContext& ctx)
    { m_Readback.ReadbackTexture(*ctx.GetCommandEncoder(), ctx.ResolveTexture(hTex)); });

  ezRenderGraphManager::EnqueueRenderGraph(m_pRenderGraph);
  m_bCaptureInFlight = true;
}

ezEnum<ezCaptureImageResult> ezWindowOutputTargetGAL::WaitCaptureImage(ezImage& out_image)
{
  if (!m_bCaptureInFlight)
    return m_bCaptureRequested ? ezCaptureImageResult::Pending : ezCaptureImageResult::NotStarted;

  ezGALDevice::GetDefaultDevice()->Flush();
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

void ezWindowOutputTargetGAL::SwapChainUpdatedEventHandler(const ezGALSwapChain* pSwapChain)
{
  if (m_hSwapChain.IsInvalidated())
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  auto* pMySwapChain = pDevice->GetSwapChain<ezGALWindowSwapChain>(m_hSwapChain);

  if (pSwapChain != pMySwapChain || !m_OnSwapChainChanged.IsValid())
    return;

  ezSizeU32 currentSize = pSwapChain->GetCurrentSize();
  if (m_Size == currentSize)
    return;

  m_Size = currentSize;
  m_OnSwapChainChanged(m_hSwapChain, currentSize);
}
