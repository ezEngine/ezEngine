
#pragma once

#include <RendererFoundation/Resources/Fence.h>

struct ID3D11Query;

class EZ_RENDERERVULKAN_DLL ezGALFenceVulkan : public ezGALFence
{
public:

  EZ_ALWAYS_INLINE ID3D11Query* GetDXFence() const
  {
    return m_pDXFence;
  }

protected:

  friend class ezGALDeviceVulkan;
  friend class ezMemoryUtils;

  ezGALFenceVulkan();

  virtual ~ezGALFenceVulkan();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11Query* m_pDXFence;
};
