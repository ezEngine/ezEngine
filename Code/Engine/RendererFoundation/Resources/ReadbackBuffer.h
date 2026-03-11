#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class ezGALDevice;

/// \brief Allows for a GPU buffer to be read back to the CPU.
/// Uses the same ezGALBufferCreationDescription as a normal buffer for convenience. While most of the properties may be irrelevant for this purpose, the user should not have to care about that and just request a readback buffer that can read back a buffer of the given description.
class EZ_RENDERERFOUNDATION_DLL ezGALReadbackBuffer : public ezGALResource<ezGALBufferCreationDescription>
{
public:
  EZ_ALWAYS_INLINE ezUInt32 GetSize() const { return m_Description.m_uiTotalSize; }

protected:
  friend class ezGALDevice;

  ezGALReadbackBuffer(const ezGALBufferCreationDescription& Description);
  virtual ~ezGALReadbackBuffer();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) = 0;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) = 0;
};
