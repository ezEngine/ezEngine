
#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALRenderTargetView : public ezGALObject<ezGALRenderTargetViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALTexture* GetTexture() const
  {
    return m_pTexture;
  }

protected:

  friend class ezGALDevice;

  ezGALRenderTargetView(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& description);

  virtual ~ezGALRenderTargetView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALTexture* m_pTexture;
};
