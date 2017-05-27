
#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALUnorderedAccessView : public ezGALObject<ezGALUnorderedAccessViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALResourceBase* GetResource() const
  {
    return m_pResource;
  }

protected:

  friend class ezGALDevice;

  ezGALUnorderedAccessView(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& description);

  virtual ~ezGALUnorderedAccessView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALResourceBase* m_pResource;
};
