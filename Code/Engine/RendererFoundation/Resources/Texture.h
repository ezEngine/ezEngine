
#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALTexture : public ezGALResource<ezGALTextureCreationDescription>
{
public:

protected:

  friend class ezGALDevice;

  ezGALTexture(const ezGALTextureCreationDescription& Description);

  virtual ~ezGALTexture();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult ReplaceExisitingNativeObject(void* pExisitingNativeObject) = 0;
};

