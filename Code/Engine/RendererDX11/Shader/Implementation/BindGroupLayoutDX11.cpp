#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/BindGroupLayoutDX11.h>

ezGALBindGroupLayoutDX11::ezGALBindGroupLayoutDX11(const ezGALBindGroupLayoutCreationDescription& Description)
  : ezGALBindGroupLayout(Description)
{
}

ezGALBindGroupLayoutDX11::~ezGALBindGroupLayoutDX11() = default;

ezResult ezGALBindGroupLayoutDX11::InitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

ezResult ezGALBindGroupLayoutDX11::DeInitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}
