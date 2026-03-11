
#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroupLayout.h>

class ezGALBindGroupLayoutDX11 : public ezGALBindGroupLayout
{
public:
protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALBindGroupLayoutDX11(const ezGALBindGroupLayoutCreationDescription& Description);

  virtual ~ezGALBindGroupLayoutDX11();
};
