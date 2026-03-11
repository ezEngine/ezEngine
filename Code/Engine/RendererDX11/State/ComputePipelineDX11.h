#pragma once

#include <Foundation/Basics.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/State/ComputePipeline.h>

class ezGALDeviceDX11;

class EZ_RENDERERDX11_DLL ezGALComputePipelineDX11 : public ezGALComputePipeline
{
public:
  ezGALComputePipelineDX11(const ezGALComputePipelineCreationDescription& description);
  ~ezGALComputePipelineDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugName(const char* szName) override;

private:
};