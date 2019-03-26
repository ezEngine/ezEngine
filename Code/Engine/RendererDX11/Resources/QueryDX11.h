#pragma once

#include <RendererFoundation/Resources/Query.h>

struct ID3D11Query;

class ezGALQueryDX11 : public ezGALQuery
{
public:

  EZ_ALWAYS_INLINE ID3D11Query* GetDXQuery() const;

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALQueryDX11(const ezGALQueryCreationDescription& Description);
  ~ezGALQueryDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Query* m_pDXQuery;
};

#include <RendererDX11/Resources/Implementation/QueryDX11_inl.h>
