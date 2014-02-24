
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALBuffer : public ezGALObjectBase<ezGALBufferCreationDescription>
{
public:

  EZ_FORCE_INLINE ezUInt32 GetSize() const;

protected:

  friend class ezGALDevice;

  ezGALBuffer(const ezGALBufferCreationDescription& Description);

  virtual ~ezGALBuffer();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, const void* pInitialData) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

#include <RendererFoundation/Resources/Implementation/Buffer_inl.h>