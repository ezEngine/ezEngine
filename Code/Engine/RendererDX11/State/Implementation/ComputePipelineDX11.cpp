#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/State/ComputePipelineDX11.h>

ezGALComputePipelineDX11::ezGALComputePipelineDX11(const ezGALComputePipelineCreationDescription& description)
  : ezGALComputePipeline(description)
{
}

ezGALComputePipelineDX11::~ezGALComputePipelineDX11() = default;

ezResult ezGALComputePipelineDX11::InitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

ezResult ezGALComputePipelineDX11::DeInitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

void ezGALComputePipelineDX11::SetDebugName(const char*)
{
}
