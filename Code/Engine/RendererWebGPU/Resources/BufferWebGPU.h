
#pragma once

#include <RendererFoundation/Resources/Buffer.h>

#include <webgpu/webgpu_cpp.h>

class EZ_RENDERERWEBGPU_DLL ezGALBufferWebGPU : public ezGALBuffer
{
protected:
  friend class ezGALDeviceWebGPU;
  friend class ezGALCommandEncoderImplWebGPU;
  friend class ezMemoryUtils;

  ezGALBufferWebGPU(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBufferWebGPU();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> initialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

private:
  wgpu::Buffer m_Buffer;
};

#include <RendererWebGPU/Resources/Implementation/BufferWebGPU_inl.h>
