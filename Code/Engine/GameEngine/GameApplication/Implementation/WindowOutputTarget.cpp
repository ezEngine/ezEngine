#include <PCH.h>

#include <Foundation/Image/Image.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

void ezWindowOutputTargetGAL::Present(bool bEnableVSync)
{
  ezGALDevice::GetDefaultDevice()->Present(m_hSwapChain, bEnableVSync);
}

ezResult ezWindowOutputTargetGAL::CaptureImage(ezImage& out_Image)
{
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

  out_Image.SetWidth(uiWidth);
  out_Image.SetHeight(uiHeight);
  out_Image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
  out_Image.AllocateImageData();
  ezUInt8* pData = out_Image.GetDataPointer<ezUInt8>();

  ezMemoryUtils::Copy(pData, backbufferData.GetData(), backbufferData.GetCount());

  return EZ_SUCCESS;
}
