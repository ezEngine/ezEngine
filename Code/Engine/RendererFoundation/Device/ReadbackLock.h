#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERFOUNDATION_DLL ezReadbackBufferLock
{
public:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReadbackBufferLock);

  ezReadbackBufferLock() = default;
  ezReadbackBufferLock(ezGALDevice* pDevice, const ezGALReadbackBuffer* pBuffer, ezArrayPtr<const ezUInt8>& out_memory);
  EZ_ALWAYS_INLINE ezReadbackBufferLock(ezReadbackBufferLock&& rhs) { *this = std::move(rhs); }
  ~ezReadbackBufferLock();

  void operator=(ezReadbackBufferLock&& rhs);

  EZ_ALWAYS_INLINE bool IsValid() const { return m_pDevice != nullptr; }
  EZ_ALWAYS_INLINE bool operator!() const { return m_pDevice == nullptr; }
  EZ_ALWAYS_INLINE operator bool() const { return m_pDevice != nullptr; }

private:
  const ezGALDevice* m_pDevice = nullptr;
  const ezGALReadbackBuffer* m_pBuffer = nullptr;
};

class EZ_RENDERERFOUNDATION_DLL ezReadbackTextureLock
{
public:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezReadbackTextureLock);

  ezReadbackTextureLock() = default;
  ezReadbackTextureLock(ezGALDevice* pDevice, const ezGALReadbackTexture* pTexture, const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_memory);
  EZ_ALWAYS_INLINE ezReadbackTextureLock(ezReadbackTextureLock&& rhs) { *this = std::move(rhs); }
  ~ezReadbackTextureLock();

  void operator=(ezReadbackTextureLock&& rhs);

  EZ_ALWAYS_INLINE bool IsValid() const { return m_pDevice != nullptr; }
  EZ_ALWAYS_INLINE bool operator!() const { return m_pDevice == nullptr; }
  EZ_ALWAYS_INLINE operator bool() const { return m_pDevice != nullptr; }

private:
  const ezGALDevice* m_pDevice = nullptr;
  const ezGALReadbackTexture* m_pTexture = nullptr;
  ezArrayPtr<const ezGALTextureSubresource> m_SubResources;
};
