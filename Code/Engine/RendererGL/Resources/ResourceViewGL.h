
#pragma once

#include <RendererFoundation/Resources/ResourceView.h>

/// \brief For GL4 glTextureView. Otherwise this class serves only for format compability checks.
class ezGALResourceViewGL : public ezGALResourceView
{
public:

protected:

  friend class ezGALDeviceGL;
  friend class ezMemoryUtils;

  ezGALResourceViewGL(const ezGALResourceViewCreationDescription& Description);

  ~ezGALResourceViewGL();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};