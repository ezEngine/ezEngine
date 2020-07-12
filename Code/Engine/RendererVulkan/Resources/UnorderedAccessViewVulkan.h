
#pragma once

#include <RendererFoundation/Resources/UnorderedAccesView.h>

struct ID3D11UnorderedAccessView;

class ezGALUnorderedAccessViewVulkan : public ezGALUnorderedAccessView
{
public:

  EZ_ALWAYS_INLINE ID3D11UnorderedAccessView* GetDXResourceView() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALUnorderedAccessViewVulkan(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description);

  ~ezGALUnorderedAccessViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11UnorderedAccessView* m_pDXUnorderedAccessView;
};

#include <RendererVulkan/Resources/Implementation/UnorderedAccessViewVulkan_inl.h>
