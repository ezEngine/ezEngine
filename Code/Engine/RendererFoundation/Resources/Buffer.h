
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class EZ_RENDERERFOUNDATION_DLL ezGALBuffer : public ezGALResource<ezGALBufferCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezUInt32 GetSize() const;
  EZ_ALWAYS_INLINE ezGALBufferRange ClampRange(ezGALBufferRange range) const;

protected:
  friend class ezGALDevice;

  ezGALBuffer(const ezGALBufferCreationDescription& Description);
  virtual ~ezGALBuffer();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<const ezUInt8> pInitialData) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>
