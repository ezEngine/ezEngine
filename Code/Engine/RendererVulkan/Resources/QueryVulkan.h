#pragma once

#include <RendererFoundation/Resources/Query.h>

struct ID3D11Query;

class ezGALQueryVulkan : public ezGALQuery
{
public:

  EZ_ALWAYS_INLINE ID3D11Query* GetDXQuery() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALQueryVulkan(const ezGALQueryCreationDescription& Description);
  ~ezGALQueryVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ID3D11Query* m_pDXQuery;
};

#include <RendererVulkan/Resources/Implementation/QueryVulkan_inl.h>
