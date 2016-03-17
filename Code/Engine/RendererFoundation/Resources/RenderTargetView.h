
#pragma once

#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

class EZ_RENDERERFOUNDATION_DLL ezGALRenderTargetView : public ezGALObject<ezGALRenderTargetViewCreationDescription>
{
public:


protected:

  friend class ezGALDevice;

  ezGALRenderTargetView(ezGALResourceBase* pResource, const ezGALRenderTargetViewCreationDescription& description);

  virtual ~ezGALRenderTargetView();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALResourceBase* m_pResource;
};
