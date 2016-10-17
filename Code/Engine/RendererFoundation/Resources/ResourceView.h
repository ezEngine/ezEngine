
#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALResourceView : public ezGALObject<ezGALResourceViewCreationDescription>
{
public:
  EZ_FORCE_INLINE ezGALResourceBase* GetResource() const
  {
    return m_pResource;
  }

protected:

  friend class ezGALDevice;

  ezGALResourceView(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& description);

  virtual ~ezGALResourceView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALResourceBase* m_pResource;
};
