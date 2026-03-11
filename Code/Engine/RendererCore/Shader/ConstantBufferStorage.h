#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

/// Wrapper around ezGALBufferHandle that automates buffer updates.
///
/// Created via ezRenderContext::CreateConstantBufferStorage. Retrieved via ezRenderContext::TryGetConstantBufferStorage,
/// updated lazily via ezRenderContext::UploadConstants. Uses hashing to avoid redundant uploads when data hasn't changed.
class EZ_RENDERERCORE_DLL ezConstantBufferStorageBase
{
protected:
  friend class ezRenderContext;
  friend class ezMemoryUtils;

  ezConstantBufferStorageBase(ezUInt32 uiSizeInBytes);
  ~ezConstantBufferStorageBase();

public:
  /// Returns writable access to the buffer data.
  ///
  /// Marks the buffer as modified for the next upload.
  ezArrayPtr<ezUInt8> GetRawDataForWriting();

  /// Returns read-only access to the buffer data.
  ezArrayPtr<const ezUInt8> GetRawDataForReading() const;

  /// Called at the beginning of each frame to reset per-frame state.
  void BeforeBeginFrame() { m_bStartOfFrame = true; }

  /// Uploads modified data to the GPU.
  ///
  /// Uses hashing to skip upload if data hasn't changed since last upload.
  void UploadData(ezGALCommandEncoder* pCommandEncoder);

  EZ_ALWAYS_INLINE ezGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool m_bHasBeenModified = false;
  bool m_bStartOfFrame = true;
  ezUInt32 m_uiLastHash = 0;
  ezGALBufferHandle m_hGALConstantBuffer;

  ezArrayPtr<ezUInt8> m_Data;
};

/// Typed wrapper for constant buffer storage.
///
/// Provides type-safe access to constant buffer data of type T.
template <typename T>
class ezConstantBufferStorage : public ezConstantBufferStorageBase
{
public:
  /// Returns a typed reference for writing to the constant buffer.
  ///
  /// Marks the buffer as modified for upload.
  EZ_FORCE_INLINE T& GetDataForWriting()
  {
    ezArrayPtr<ezUInt8> rawData = GetRawDataForWriting();
    EZ_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  /// Returns a typed const reference for reading from the constant buffer.
  EZ_FORCE_INLINE const T& GetDataForReading() const
  {
    ezArrayPtr<const ezUInt8> rawData = GetRawDataForReading();
    EZ_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

using ezConstantBufferStorageId = ezGenericId<24, 8>;

class ezConstantBufferStorageHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezConstantBufferStorageHandle, ezConstantBufferStorageId);

  friend class ezRenderContext;
};
