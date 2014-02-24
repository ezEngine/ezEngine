
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALResourceView : public ezGALObjectBase<ezGALResourceViewCreationDescription>
{
public:


protected:

  friend class ezGALDevice;

  ezGALResourceView(const ezGALResourceViewCreationDescription& Description);

  virtual ~ezGALResourceView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

};
