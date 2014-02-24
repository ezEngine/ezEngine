
#pragma once

#include <RendererFoundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALRenderTargetView : public ezGALObjectBase<ezGALRenderTargetViewCreationDescription>
{
public:


protected:

  friend class ezGALDevice;

  ezGALRenderTargetView(const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

};
