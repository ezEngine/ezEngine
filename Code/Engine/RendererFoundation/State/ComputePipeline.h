#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALComputePipeline : public ezGALObject<ezGALComputePipelineCreationDescription>
{
public:
  ezGALComputePipeline(const ezGALComputePipelineCreationDescription& description)
    : ezGALObject<ezGALComputePipelineCreationDescription>(description)
  {
  }

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
  virtual void SetDebugName(const char* szName) = 0;
};
