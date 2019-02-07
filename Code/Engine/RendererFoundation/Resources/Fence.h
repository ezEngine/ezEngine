
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezGALFence : public ezRefCounted
{
protected:

  friend class ezGALDevice;

  ezGALFence();

  virtual ~ezGALFence();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};

