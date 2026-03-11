#include <RendererDX11/Shader/BindGroupDX11.h>

ezGALBindGroupDX11::ezGALBindGroupDX11(const ezGALBindGroupCreationDescription& Description)
  : ezGALBindGroup(Description)
{
}

ezGALBindGroupDX11::~ezGALBindGroupDX11() = default;

ezResult ezGALBindGroupDX11::InitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

ezResult ezGALBindGroupDX11::DeInitPlatform(ezGALDevice*)
{
  return EZ_SUCCESS;
}

void ezGALBindGroupDX11::Invalidate(ezGALDevice*)
{
  m_bInvalidated = true;
}

bool ezGALBindGroupDX11::IsInvalidated() const
{
  return m_bInvalidated;
}

void ezGALBindGroupDX11::SetDebugNamePlatform(const char*) const
{
}
