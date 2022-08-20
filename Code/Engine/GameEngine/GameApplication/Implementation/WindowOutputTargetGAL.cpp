#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
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
    ezGALDevice::GetDefaultDevice()->UpdateSwapChain(m_hSwapChain, ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate).AssertSuccess("");

    if (bSwapChainExisted && m_OnSwapChainChanged.IsValid())
    {
      m_OnSwapChainChanged(m_hSwapChain, m_Size);
    }
  }
  else
  {
    m_hSwapChain = ezGALWindowSwapChain::Create(m_currentDesc);
  }
}

void ezWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  // Only re-create the swapchain if somebody is listening to changes.
  if (m_OnSwapChainChanged.IsValid())
  {
    ezEnum<ezGALPresentMode> presentMode = ezGameApplication::cvar_AppVSync ? ezGALPresentMode::VSync : ezGALPresentMode::Immediate;

    // The actual present call is done by setting the swapchain to an ezView.
    // This call is only used to recreate the swapchain at a safe location.
    if (m_Size != m_currentDesc.m_pWindow->GetClientAreaSize() || presentMode != m_currentDesc.m_InitialPresentMode)
    {
      CreateSwapchain(m_currentDesc);
    }
  }
}

ezResult ezWindowOutputTargetGAL::CaptureImage(ezImage& out_Image)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("CaptureImage");
  EZ_SCOPE_EXIT(pDevice->EndPass(pGALPass));

  auto pGALCommandEncoder = pGALPass->BeginRendering(ezGALRenderingSetup());
  EZ_SCOPE_EXIT(pGALPass->EndRendering(pGALCommandEncoder));

  const ezGALSwapChain* pSwapChain = pDevice->GetSwapChain(m_hSwapChain);
  ezGALTextureHandle hBackbuffer = pSwapChain ? pSwapChain->GetRenderTargets().m_hRTs[0] : ezGALTextureHandle();

  pGALCommandEncoder->ReadbackTexture(hBackbuffer);

  const ezGALTexture* pBackbuffer = ezGALDevice::GetDefaultDevice()->GetTexture(hBackbuffer);
  const ezUInt32 uiWidth = pBackbuffer->GetDescription().m_uiWidth;
  const ezUInt32 uiHeight = pBackbuffer->GetDescription().m_uiHeight;

  ezDynamicArray<ezUInt8> backbufferData;
  backbufferData.SetCountUninitialized(uiWidth * uiHeight * 4);

  ezGALSystemMemoryDescription MemDesc;
  MemDesc.m_uiRowPitch = 4 * uiWidth;
  MemDesc.m_uiSlicePitch = 4 * uiWidth * uiHeight;

  /// \todo Make this more efficient
  MemDesc.m_pData = backbufferData.GetData();
  ezArrayPtr<ezGALSystemMemoryDescription> SysMemDescsDepth(&MemDesc, 1);
  ezGALTextureSubresource sourceSubResource;
  ezArrayPtr<ezGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);
  pGALCommandEncoder->CopyTextureReadbackResult(hBackbuffer, sourceSubResources, SysMemDescsDepth);

  ezImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  out_Image.ResetAndAlloc(header);
  ezUInt8* pData = out_Image.GetPixelPointer<ezUInt8>();

  ezMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
