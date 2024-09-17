#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/BufferWebGPU.h>
#include <RendererWebGPU/Resources/TextureWebGPU.h>
#include <RendererWebGPU/Resources/UnorderedAccessViewWebGPU.h>

ezGALTextureUnorderedAccessViewWebGPU::ezGALTextureUnorderedAccessViewWebGPU(
  ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description)
  : ezGALTextureUnorderedAccessView(pResource, Description)
{
}

ezGALTextureUnorderedAccessViewWebGPU::~ezGALTextureUnorderedAccessViewWebGPU() = default;

ezResult ezGALTextureUnorderedAccessViewWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALTextureUnorderedAccessViewWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

/////////////////////////////////////////////////////////////////////

ezGALBufferUnorderedAccessViewWebGPU::ezGALBufferUnorderedAccessViewWebGPU(
  ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description)
  : ezGALBufferUnorderedAccessView(pResource, Description)
{
}

ezGALBufferUnorderedAccessViewWebGPU::~ezGALBufferUnorderedAccessViewWebGPU() = default;

ezResult ezGALBufferUnorderedAccessViewWebGPU::InitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}

ezResult ezGALBufferUnorderedAccessViewWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}
