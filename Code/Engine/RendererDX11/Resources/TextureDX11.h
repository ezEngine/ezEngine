#pragma once

#include <RendererFoundation/Resources/Texture.h>


struct ID3D11Resource;

class ezGALTextureDX11 : public ezGALTexture
{
public:

  EZ_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  EZ_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALTextureDX11(const ezGALTextureCreationDescription& Description);

  ~ezGALTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult ReplaceExisitingNativeObject(void* pExisitingNativeObject) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult CreateStagingTexture(ezGALDeviceDX11* pDevice);

  ID3D11Resource* m_pDXTexture;

  ID3D11Resource* m_pDXStagingTexture;

  void* m_pExisitingNativeObject = nullptr;

};

#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
