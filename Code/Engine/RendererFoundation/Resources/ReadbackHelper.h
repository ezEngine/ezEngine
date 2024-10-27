#pragma once

#include <RendererFoundation/Device/ReadbackLock.h>

class ezGALDevice;

/// \brief Base-class for ezGALReadbackBufferHelper and ezGALReadbackTextureHelper.
class EZ_RENDERERFOUNDATION_DLL ezGALReadbackHelper
{
public:
  /// \brief Returns the fence of the last readback operation.
  EZ_FORCE_INLINE ezGALFenceHandle GetCurrentFence() const { return m_hFence; }
  /// \brief Returns the current status of the readback.
  [[nodiscard]] ezEnum<ezGALAsyncResult> GetReadbackResult(ezTime timeout) const;

protected:
  ezGALDevice* m_pDevice = nullptr;
  ezGALFenceHandle m_hFence = 0;
};

/// \brief Helper class that automatically creates a readback buffer and controls it's lifetime.
class EZ_RENDERERFOUNDATION_DLL ezGALReadbackBufferHelper : public ezGALReadbackHelper
{
public:
  ezGALReadbackBufferHelper() = default;
  ~ezGALReadbackBufferHelper();

  /// \brief Free the memory of the readback buffer.
  void Reset();
  /// \brief Starts a readback of a buffer. A new readback buffer will be created if the current one does not match the new buffer.
  ezGALFenceHandle ReadbackBuffer(ezGALCommandEncoder& encoder, ezGALBufferHandle hBuffer);
  /// \brief Same as ezGALDevice::LockBuffer, but checks that the fence has been reached. If not, returns an invalid lock object.
  ezReadbackBufferLock LockBuffer(ezArrayPtr<const ezUInt8>& out_Memory);

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALReadbackBufferHelper);
  ezGALReadbackBufferHandle m_hReadbackBuffer;
};

/// \brief Helper class that automatically creates a readback texture and controls it's lifetime.
class EZ_RENDERERFOUNDATION_DLL ezGALReadbackTextureHelper : public ezGALReadbackHelper
{
public:
  ezGALReadbackTextureHelper() = default;
  ~ezGALReadbackTextureHelper();

  /// \brief Free the memory of the readback texture.
  void Reset();
  /// \brief Starts a readback of a texture. A new readback texture will be created if the current one does not match the new texture.
  ezGALFenceHandle ReadbackTexture(ezGALCommandEncoder& encoder, ezGALTextureHandle hTexture);
  /// \brief Same as ezGALDevice::LockTexture, but checks that the fence has been reached. If not, returns an invalid lock object.
  ezReadbackTextureLock LockTexture(const ezArrayPtr<const ezGALTextureSubresource>& subResources, ezDynamicArray<ezGALSystemMemoryDescription>& out_Memory);

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGALReadbackTextureHelper);
  ezGALReadbackTextureHandle m_hReadbackTexture;
};
