
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALBindGroupLayout : public ezGALObject<ezGALBindGroupLayoutCreationDescription>
{
protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALBindGroupLayout(const ezGALBindGroupLayoutCreationDescription& Description);
  virtual ~ezGALBindGroupLayout();
};
