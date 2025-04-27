#pragma once

#include <Foundation/Basics.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// \brief Graphics pipeline state object combining shader, blend, depth-stencil, rasterizer state, and vertex declaration into a single object.
class EZ_RENDERERFOUNDATION_DLL ezGALGraphicsPipeline : public ezGALObject<ezGALGraphicsPipelineCreationDescription>
{
public:
  ezGALGraphicsPipeline(const ezGALGraphicsPipelineCreationDescription& description)
    : ezGALObject<ezGALGraphicsPipelineCreationDescription>(description)
  {
  }

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
  virtual void SetDebugName(const char* szName) = 0;
};
