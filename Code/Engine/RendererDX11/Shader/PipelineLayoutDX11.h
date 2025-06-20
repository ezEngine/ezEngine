#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/PipelineLayout.h>
#include <RendererDX11/RendererDX11DLL.h>

class ezGALPipelineLayoutDX11 : public ezGALPipelineLayout
{
public:

protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  ezGALPipelineLayoutDX11(const ezGALPipelineLayoutCreationDescription& Description);

  virtual ~ezGALPipelineLayoutDX11();

};
