#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class ezGALTextureDX11 : public ezGALTexture
{
public:

  EZ_FORCE_INLINE ID3D11Resource* GetDXTexture() const;

  EZ_FORCE_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALTextureDX11(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezResult CreateStagingTexture(ezGALDeviceDX11* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

};

#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>