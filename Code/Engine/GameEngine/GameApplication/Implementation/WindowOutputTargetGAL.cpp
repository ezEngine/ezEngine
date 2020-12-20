#include <GameEnginePCH.h>

#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>
#include <Texture/Image/Image.h>

ezWindowOutputTargetGAL::ezWindowOutputTargetGAL() {}

ezWindowOutputTargetGAL::~ezWindowOutputTargetGAL()
{
  // do not try to destroy the primary swapchain, that is handled by the device
  if (ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain() != m_hSwapChain)
  {
    ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  }

  m_hSwapChain.Invalidate();
}

void ezWindowOutputTargetGAL::CreateSwapchain(const ezGALSwapChainCreationDescription& desc)
{
  m_hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);
}

void ezWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  ezGALDevice::GetDefaultDevice()->Present(m_hSwapChain, bEnableVSync);
}

ezResult ezWindowOutputTargetGAL::CaptureImage(ezImage& out_Image)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
#if 0
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALTextureHandle hBackbuffer = pDevice->GetBackBufferTextureFromSwapChain(m_hSwapChain);

  ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->ReadbackTexture(hBackbuffer);

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
  ezGALDevice::GetDefaultDevice()->GetPrimaryContext()->CopyTextureReadbackResult(hBackbuffer, &SysMemDescsDepth);

  ezImageHeader header;
  header.SetWidth(uiWidth);
  header.SetHeight(uiHeight);
  header.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  out_Image.ResetAndAlloc(header);
  ezUInt8* pData = out_Image.GetPixelPointer<ezUInt8>();

  ezMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());
#endif

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_GameApplication_Implementation_WindowOutputTargetGAL);
