
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class EZ_RENDERERFOUNDATION_DLL ezGALProxyTexture : public ezGALTexture
{
public:
  virtual ~ezGALProxyTexture();

  virtual const ezGALResourceBase* GetParentResource() const override;

protected:
  friend class ezGALDevice;

  ezGALProxyTexture(const ezGALTexture& parentTexture);  

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult ReplaceExisitingNativeObject(void* pExisitingNativeObject) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  const ezGALTexture* m_pParentTexture;
};

