
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class EZ_RENDERERFOUNDATION_DLL ezGALResourceView : public ezGALObject<ezGALResourceViewCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class ezGALDevice;

  ezGALResourceView(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& description);

  virtual ~ezGALResourceView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALResourceBase* m_pResource;
};
