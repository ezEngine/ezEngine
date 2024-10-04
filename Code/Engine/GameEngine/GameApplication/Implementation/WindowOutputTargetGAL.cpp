#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererCore/Pipeline/Declarations.h>
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
}

ezWindowOutputTargetGAL::~ezWindowOutputTargetGAL()
{
  ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();
  // After the swapchain is destroyed it can still be used in the renderer. As right after this usually the window is destroyed we must ensure that nothing still renders to it.
  ezGALDevice::GetDefaultDevice()->WaitIdle();
}

void ezWindowOutputTargetGAL::CreateSwapchain(const ezGALWindowSwapChainCreationDescription& desc)
{
  m_currentDesc = desc;
  // ezWindowOutputTargetGAL takes over the present mode and keeps it up to date with cvar_AppVSync.
  m_Size = desc.m_pWindow->GetClientAreaSize();
  m_currentDesc.m_InitialPresentMode = ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate;

  const bool bSwapChainExisted = !m_hSwapChain.IsInvalidated();
  if (bSwapChainExisted)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    auto* pSwapchain = pDevice->GetSwapChain<ezGALWindowSwapChain>(m_hSwapChain);
    pDevice->UpdateSwapChain(m_hSwapChain, ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate).AssertSuccess("");
    if (bSwapChainExisted && m_OnSwapChainChanged.IsValid())
    {
      // The swapchain may have a different size than the window advertised, e.g. if the window has been resized further in the meantime.
      ezSizeU32 currentSize = pSwapchain->GetCurrentSize();
      m_OnSwapChainChanged(m_hSwapChain, currentSize);
    }
  }
  else
  {
    m_hSwapChain = ezGALWindowSwapChain::Create(m_currentDesc);
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

  // Only re-create the swapchain if somebody is listening to changes.
  if (m_OnSwapChainChanged.IsValid())
  {
    ezEnum<ezGALPresentMode> presentMode = ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate;

    if (m_Size != m_currentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_currentDesc.m_InitialPresentMode)
    {
      CreateSwapchain(m_currentDesc);
    }
  }
}

ezResult ezWindowOutputTargetGAL::CaptureImage(ezImage& out_image)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALTextureHandle hBackbuffer;
  // Start readback image
  {
    auto pCommandEncoder = pDevice->BeginCommands("CaptureImage");
    EZ_SCOPE_EXIT(pDevice->EndCommands(pCommandEncoder));

    const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
    hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : ezGALTextureHandle();
    m_Readback.ReadbackTexture(*pCommandEncoder, hBackbuffer);
  }
  // Wait for results
  {
    ezEnum<ezGALAsyncResult> res = m_Readback.GetReadbackResult(ezTime::MakeFromHours(1));
    EZ_ASSERT_ALWAYS(res == ezGALAsyncResult::Ready, "Readback of texture failed");
  }

  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  ezHybridArray<ezGALSystemMemoryDescription, 1> memory;
  m_Readback.LockTexture(sourceSubResources, memory).AssertSuccess("Failed to lock readback texture");
  EZ_SCOPE_EXIT(m_Readback.UnlockTexture(sourceSubResources).AssertSuccess(""));

  const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  ezTextureUtils::CreateSubResourceImage(pBackbuffer->GetDescription(), sourceSubResource, memory[0], out_image);
  return EZ_SUCCESS;
}
