#pragma once

#include <Foundation/Basics.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/State/GraphicsPipeline.h>

class ezGALDeviceDX11;

class EZ_RENDERERDX11_DLL ezGALGraphicsPipelineDX11 : public ezGALGraphicsPipeline
{
public:
  ezGALGraphicsPipelineDX11(const ezGALGraphicsPipelineCreationDescription& description);
  ~ezGALGraphicsPipelineDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugName(const char* szName) override;

private:
};