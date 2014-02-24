
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALRenderTargetConfig : public ezGALObjectBase<ezGALRenderTargetConfigCreationDescription>
{
public:


protected:

  friend class ezGALDevice;

  ezGALRenderTargetConfig(const ezGALRenderTargetConfigCreationDescription& Description);

  virtual ~ezGALRenderTargetConfig();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

};
