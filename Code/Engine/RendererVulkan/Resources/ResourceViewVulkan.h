
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

struct ID3D11ShaderResourceView;

class ezGALResourceViewVulkan : public ezGALResourceView
{
public:

  EZ_ALWAYS_INLINE ID3D11ShaderResourceView* GetDXResourceView() const;

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALResourceViewVulkan(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description);

  ~ezGALResourceViewVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11ShaderResourceView* m_pDXResourceView;
};

#include <RendererVulkan/Resources/Implementation/ResourceViewVulkan_inl.h>
