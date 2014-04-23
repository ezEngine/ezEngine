
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class ezGALResourceViewDX11 : public ezGALResourceView
{
public:

  EZ_FORCE_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALResourceViewDX11(const ezGALResourceViewCreationDescription& Description);

  ~ezGALResourceViewDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView;
};

#include <RendererDX11/Resources/Implementation/ResourceViewDX11_inl.h>
