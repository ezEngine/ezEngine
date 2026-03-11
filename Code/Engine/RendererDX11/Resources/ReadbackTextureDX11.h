#pragma once

#include <RendererFoundation/Resources/ReadbackTexture.h>

struct ID3D11Resource;
struct D3D11_TEXTURE2D_DESC;
struct D3D11_TEXTURE3D_DESC;
struct D3D11_SUBRESOURCE_DATA;
class ezGALDeviceDX11;

class ezGALReadbackTextureDX11 : public ezGALReadbackTexture
{
public:
  EZ_ALWAYS_INLINE ID3D11Resource* GetDXTexture() const { return m_pDXTexture; }

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALReadbackTextureDX11(const ezGALTextureCreationDescription& Description);
  ~ezGALReadbackTextureDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

protected:
  ID3D11Resource* m_pDXTexture = nullptr;
};
