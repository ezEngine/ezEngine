
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class ezGALTexture;
class ezGALBuffer;

class EZ_RENDERERFOUNDATION_DLL ezGALTextureUnorderedAccessView : public ezGALObject<ezGALTextureUnorderedAccessViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class ezGALDevice;

  ezGALTextureUnorderedAccessView(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& description);

  virtual ~ezGALTextureUnorderedAccessView();
  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALTexture* m_pResource;
};

class EZ_RENDERERFOUNDATION_DLL ezGALBufferUnorderedAccessView : public ezGALObject<ezGALBufferUnorderedAccessViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALBuffer* GetResource() const { return m_pResource; }
  
protected:
  friend class ezGALDevice;

  ezGALBufferUnorderedAccessView(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& description);

  virtual ~ezGALBufferUnorderedAccessView();
  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALBuffer* m_pResource;
};