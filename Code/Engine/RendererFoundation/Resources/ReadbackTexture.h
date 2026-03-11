#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class ezGALDevice;

/// \brief Allows for a GPU texture to be read back to the CPU.
/// Uses the same ezGALTextureCreationDescription as a normal texture for convenience. While most of the properties may be irrelevant for this purpose, the user should not have to care about that and just request a readback texture that can read back a texture of the given description.
class EZ_RENDERERFOUNDATION_DLL ezGALReadbackTexture : public ezGALResource<ezGALTextureCreationDescription>
{
protected:
  friend class ezGALDevice;

  ezGALReadbackTexture(const ezGALTextureCreationDescription& Description);
  virtual ~ezGALReadbackTexture();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};
