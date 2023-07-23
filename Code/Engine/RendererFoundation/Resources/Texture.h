
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

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
  ezUInt64 a = 0;
  ezUInt64 b = 0;
  ezUInt32 m_uiProcessId;
  ezUInt32 m_uiMemoryTypeIndex;
  ezUInt64 m_uiSize;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERFOUNDATION_DLL, ezGALPlatformSharedHandle);

// Optional interface for ezGALTexture
// A ezGALTexture can be a shared texture, but doesn't have to be
// Access through ezGALDevice::GetSharedTexture
class EZ_RENDERERFOUNDATION_DLL ezGALSharedTexture
{
protected:
  virtual ~ezGALSharedTexture();

  public:

    virtual ezGALPlatformSharedHandle GetSharedHandle() const = 0;
    /// \brief Before the current render pipeline is executed, the GPU will wait for the semaphore to have the given value.  
    /// \param iValue Value the semaphore needs to have before the texture can be used.
    virtual void WaitSemaphoreGPU(ezUInt64 uiValue) const = 0;
    /// \brief Once the current render pipeline is done on the GPU, the semaphore will be signaled with the given value.
    /// \param iValue Value the semaphore is set to once we are done using the texture (after the current render pipeline).
    virtual void SignalSemaphoreGPU(ezUInt64 uiValue) const = 0;
};
