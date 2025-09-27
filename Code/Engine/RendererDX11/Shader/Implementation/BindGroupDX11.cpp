#include <RendererDX11/Shader/BindGroupDX11.h>

ezGALBindGroupDX11::ezGALBindGroupDX11(const ezGALBindGroupCreationDescription& Description)
  : ezGALBindGroup(Description)
{
}

ezGALBindGroupDX11::~ezGALBindGroupDX11() = default;

ezResult ezGALBindGroupDX11::InitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

ezResult ezGALBindGroupDX11::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

void ezGALBindGroupDX11::Invalidate(ezGALDevice* pDevice)
{
  m_bInvalidated = true;
}

bool ezGALBindGroupDX11::IsInvalidated() const
{
  return m_bInvalidated;
}

void ezGALBindGroupDX11::SetDebugNamePlatform(const char* szName) const
{
}
