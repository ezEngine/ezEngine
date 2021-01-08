#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <RendererCore/RendererCoreDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class EZ_RENDERERCORE_DLL ezConstantBufferStorageBase
{
protected:
  friend class ezRenderContext;
  friend class ezMemoryUtils;

  ezConstantBufferStorageBase(ezUInt32 uiSizeInBytes);
  ~ezConstantBufferStorageBase();

public:
  ezArrayPtr<ezUInt8> GetRawDataForWriting();
  ezArrayPtr<const ezUInt8> GetRawDataForReading() const;

  void UploadData(ezGALCommandEncoder* pCommandEncoder);

  EZ_ALWAYS_INLINE ezGALBufferHandle GetGALBufferHandle() const { return m_hGALConstantBuffer; }

protected:
  bool m_bHasBeenModified;
  ezUInt32 m_uiLastHash;
  ezGALBufferHandle m_hGALConstantBuffer;

  ezArrayPtr<ezUInt8> m_Data;
};

template <typename T>
class ezConstantBufferStorage : public ezConstantBufferStorageBase
{
public:
  EZ_FORCE_INLINE T& GetDataForWriting()
  {
    ezArrayPtr<ezUInt8> rawData = GetRawDataForWriting();
    EZ_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<T*>(rawData.GetPtr());
  }

  EZ_FORCE_INLINE const T& GetDataForReading() const
  {
    ezArrayPtr<const ezUInt8> rawData = GetRawDataForReading();
    EZ_ASSERT_DEV(rawData.GetCount() == sizeof(T), "Invalid data size");
    return *reinterpret_cast<const T*>(rawData.GetPtr());
  }
};

typedef ezGenericId<24, 8> ezConstantBufferStorageId;

class ezConstantBufferStorageHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezConstantBufferStorageHandle, ezConstantBufferStorageId);

  friend class ezRenderContext;
};
