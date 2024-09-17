#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/RenderTargetViewWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>

ezGALRenderTargetViewWebGPU::ezGALRenderTargetViewWebGPU(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(pTexture, Description)

{
}

ezGALRenderTargetViewWebGPU::~ezGALRenderTargetViewWebGPU() = default;

ezResult ezGALRenderTargetViewWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALRenderTargetViewWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}


