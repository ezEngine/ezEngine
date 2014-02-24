
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALTexture : public ezGALObjectBase<ezGALTextureCreationDescription>
{
public:

protected:

  friend class ezGALDevice;

  ezGALTexture(const ezGALTextureCreationDescription& Description);

  virtual ~ezGALTexture();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

//#include <RendererFoundation/Resource/Implementation/Texture_inl.h>