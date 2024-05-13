
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class ezGALTexture;
class ezGALBuffer;

class EZ_RENDERERFOUNDATION_DLL ezGALTextureResourceView : public ezGALObject<ezGALTextureResourceViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALTexture* GetResource() const { return m_pResource; }

protected:
  friend class ezGALDevice;

  ezGALTextureResourceView(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& description);

  virtual ~ezGALTextureResourceView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALTexture* m_pResource;
};

class EZ_RENDERERFOUNDATION_DLL ezGALBufferResourceView : public ezGALObject<ezGALBufferResourceViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALBuffer* GetResource() const { return m_pResource; }

protected:
  friend class ezGALDevice;

  ezGALBufferResourceView(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& description);

  virtual ~ezGALBufferResourceView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALBuffer* m_pResource;
};
