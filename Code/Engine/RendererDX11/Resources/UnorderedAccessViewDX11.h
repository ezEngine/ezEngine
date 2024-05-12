
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class ezGALTextureUnorderedAccessViewDX11 : public ezGALTextureUnorderedAccessView
{
public:
  EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALTextureUnorderedAccessViewDX11(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& Description);
  ~ezGALTextureUnorderedAccessViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView = nullptr;
};

class ezGALBufferUnorderedAccessViewDX11 : public ezGALBufferUnorderedAccessView
{
public:
  EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALBufferUnorderedAccessViewDX11(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& Description);
  ~ezGALBufferUnorderedAccessViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView = nullptr;
};

#include <RendererDX11/Resources/Implementation/UnorderedAccessViewDX11_inl.h>
