
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class ezGALTextureResourceViewDX11 : public ezGALTextureResourceView
{
public:
  EZ_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALTextureResourceViewDX11(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& Description);

  ~ezGALTextureResourceViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView = nullptr;
};


class ezGALBufferResourceViewDX11 : public ezGALBufferResourceView
{
public:
  EZ_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALBufferResourceViewDX11(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& Description);

  ~ezGALBufferResourceViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView = nullptr;
};

#include <RendererDX11/Resources/Implementation/ResourceViewDX11_inl.h>
