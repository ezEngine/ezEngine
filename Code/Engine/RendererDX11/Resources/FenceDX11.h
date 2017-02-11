
#pragma once

#include <RendererFoundation/Resources/Fence.h>

struct ID3D11Query;

class EZ_RENDERERDX11_DLL ezGALFenceDX11 : public ezGALFence
{
public:

  EZ_FORCE_INLINE ID3D11Query* GetDXFence() const
  {
    return m_pDXFence;
  }

protected:

  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  ezGALFenceDX11();

  virtual ~ezGALFenceDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ID3D11Query* m_pDXFence;
};
