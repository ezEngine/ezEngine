
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class EZ_RENDERERFOUNDATION_DLL ezGALProxyTexture : public ezGALTexture
{
public:
  virtual ~ezGALProxyTexture();

protected:
  friend class ezGALDevice;

  ezGALProxyTexture(const ezGALTexture& parentTexture);  

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugName(const char* szName) const override;
};

