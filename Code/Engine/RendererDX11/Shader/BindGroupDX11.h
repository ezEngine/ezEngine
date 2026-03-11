#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/BindGroup.h>

class ezGALBindGroupDX11 : public ezGALBindGroup
{
public:
protected:
  friend class ezGALDeviceDX11;
  friend class ezMemoryUtils;

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void Invalidate(ezGALDevice* pDevice) override;
  virtual bool IsInvalidated() const override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezGALBindGroupDX11(const ezGALBindGroupCreationDescription& Description);

  virtual ~ezGALBindGroupDX11();

private:
  bool m_bInvalidated = false;
};
