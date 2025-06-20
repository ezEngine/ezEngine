#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALPipelineLayout : public ezGALObject<ezGALPipelineLayoutCreationDescription>
{
public:
protected:
  friend class ezGALDevice;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;

  ezGALPipelineLayout(const ezGALPipelineLayoutCreationDescription& Description);
  virtual ~ezGALPipelineLayout();

private:
};
