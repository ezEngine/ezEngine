#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALQuery : public ezGALResource<ezGALQueryCreationDescription>
{
public:

protected:

  friend class ezGALDevice;
  friend class ezGALContext;

  ezGALQuery(const ezGALQueryCreationDescription& Description);

  virtual ~ezGALQuery();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  bool m_bStarted;
};

