
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class EZ_RENDERERFOUNDATION_DLL ezGALTexture : public ezGALResource<ezGALTextureCreationDescription>
{
public:
protected:
  friend class ezGALDevice;

  ezGALTexture(const ezGALTextureCreationDescription& Description);

  virtual ~ezGALTexture();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) = 0;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};


// Type of the shared texture
enum class ezGALSharedTextureType : ezUInt8
{
  None,     ///< Not shared
  Exported, ///< Allocation owned by this process
  Imported  ///< Allocation owned by a different process
};

// Opaque platform specific handle
// Typically holds a platform specific handle for the texture and it's synchronisation primitive
struct ezGALPlatformSharedHandle
{
  size_t a = 0;
  size_t b = 0;
};

// Optional interface for ezGALTexture
// A ezGALTexture can be a shared texture, but doesn't have to be
// Access through ezGALDevice::GetSharedTexture
class EZ_RENDERERFOUNDATION_DLL ezGALSharedTexture
{
protected:
  virtual ~ezGALSharedTexture();

  public:

    virtual ezGALPlatformSharedHandle GetSharedHandle() const = 0;
};