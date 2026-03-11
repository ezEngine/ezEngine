#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/PipelineLayoutDX11.h>

ezGALPipelineLayoutDX11::ezGALPipelineLayoutDX11(const ezGALPipelineLayoutCreationDescription& Description)
  : ezGALPipelineLayout(Description)
{
}

ezGALPipelineLayoutDX11::~ezGALPipelineLayoutDX11() = default;

ezResult ezGALPipelineLayoutDX11::InitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

ezResult ezGALPipelineLayoutDX11::DeInitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}
