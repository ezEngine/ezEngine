#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class ezGALDeviceDX11;

EZ_DEFINE_AS_POD_TYPE(D3D11_SUBRESOURCE_DATA);

class ezGALTextureDX11 : public ezGALTexture
{
public:
  EZ_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const;

  EZ_ALWAYS_INLINE ID3D11Resource* GetDXStagingTexture() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;
  friend class ezGALSharedTextureDX11;

  ezGALTextureDX11(const ezGALTextureCreationDescription& Description);
  ~ezGALTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult InitFromNativeObject(ezGALDeviceDX11* pDXDevice);
  static ezResult Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE2D_DESC& out_Tex2DDesc);
  static ezResult Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX11* pDXDevice, D3D11_TEXTURE3D_DESC& out_Tex3DDesc);
  static void ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezHybridArray<D3D11_SUBRESOURCE_DATA, 16>& out_InitialData);

protected:
  ID3D11Resource* m_pDXTexture = nullptr;
};



#include <RendererDX11/Resources/Implementation/TextureDX11_inl.h>
