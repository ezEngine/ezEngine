
#pragma once

#include <RendererFoundation/Resources/Texture.h>

class EZ_RENDERERFOUNDATION_DLL ezGALProxyTexture : public ezGALTexture
{
public:
  virtual ~ezGALProxyTexture();

  virtual const ezGALResourceBase* GetParentResource() const override;
  ezGALTextureHandle GetParentTextureHandle() const { return m_hParentTexture; }
  ezUInt32 GetSlice() const { return m_uiSlice; }
protected:
  friend class ezGALDevice;

  ezGALProxyTexture(ezGALTextureHandle hParentTexture, const ezGALTexture& parentTexture, ezUInt32 uiSlice);

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezGALTextureHandle m_hParentTexture;
  const ezGALTexture* m_pParentTexture;
  ezUInt32 m_uiSlice = 0;
};
