#include <RendererWebGPU/RendererWebGPUPCH.h>

#include <RendererWebGPU/Device/DeviceWebGPU.h>
#include <RendererWebGPU/Resources/SharedTextureWebGPU.h>

//////////////////////////////////////////////////////////////////////////
// ezGALSharedTextureWebGPU
//////////////////////////////////////////////////////////////////////////

ezGALSharedTextureWebGPU::ezGALSharedTextureWebGPU(const ezGALTextureCreationDescription& Description, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle hSharedHandle)
  : ezGALTextureWebGPU(Description)
{
}

ezGALSharedTextureWebGPU::~ezGALSharedTextureWebGPU() = default;

ezResult ezGALSharedTextureWebGPU::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_WEBGPU_TRACE();
  return EZ_SUCCESS;
}


ezResult ezGALSharedTextureWebGPU::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_WEBGPU_TRACE();

  return SUPER::DeInitPlatform(pDevice);
}

ezGALPlatformSharedHandle ezGALSharedTextureWebGPU::GetSharedHandle() const
{
  EZ_WEBGPU_TRACE();
  return {};
}

void ezGALSharedTextureWebGPU::WaitSemaphoreGPU(ezUInt64 uiValue) const
{
  EZ_WEBGPU_TRACE();
}

void ezGALSharedTextureWebGPU::SignalSemaphoreGPU(ezUInt64 uiValue) const
{
  EZ_WEBGPU_TRACE();
}
