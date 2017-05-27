
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class ezGALUnorderedAccessViewDX11 : public ezGALUnorderedAccessView
{
public:

  EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALUnorderedAccessViewDX11(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description);

  ~ezGALUnorderedAccessViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView;
};

#include <RendererDX11/Resources/Implementation/UnorderedAccessViewDX11_inl.h>
