#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>

class ezGALDynamicBufferHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezGALDynamicBufferHandle, ezGAL::ez18_14Id);

  friend class ezGALDynamicBuffer;
};


class EZ_RENDERERFOUNDATION_DLL ezGALDynamicBuffer
{
public:
  static ezGALDynamicBufferHandle Create(const ezGALBufferCreationDescription& desc, ezStringView sDebugName);
  static void Destroy(ezGALDynamicBufferHandle& inout_hBuffer);
  static ezGALDynamicBuffer* Get(ezGALDynamicBufferHandle hBuffer);


  template <typename U>
  ezUInt32 Allocate(const U& userData, ezUInt32 uiCount)
  {
    static_assert(sizeof(U) <= sizeof(ezUInt64), "userData is too large");
    ezUInt64 uiUserData = 0;
    *reinterpret_cast<U*>(&uiUserData) = userData;

    return Allocate(uiUserData, uiCount);
  }

  void Deallocate(ezUInt32 uiOffset);

  template <typename T>
  ezArrayPtr<T> MapForWriting(ezUInt32 uiOffset)
  {
    ezUInt32 uiCount = 0;
    ezByteArrayPtr byteData = MapForWriting(uiOffset, uiCount);
    EZ_ASSERT_DEBUG(uiCount * m_Desc.m_uiStructSize == byteData.GetCount(), "Invalid Type");
    return ezArrayPtr<T>(reinterpret_cast<T*>(byteData.GetPtr()), uiCount);
  }

  void UploadChanges();

  struct ChangedAllocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiNewOffset = 0;
  };

  void RunCompactionSteps(ezDynamicArray<ChangedAllocation>& out_changedAllocations, ezUInt32 uiMaxSteps = 16);

  const ezGALBufferHandle& GetBufferForRendering() const
  {
    return m_hBufferForRendering;
  }

private:
  friend class ezMemoryUtils;
  friend class ezGALDevice;

  ezGALDynamicBuffer() = default;
  ~ezGALDynamicBuffer();

  void Initialize(const ezGALBufferCreationDescription& desc, ezStringView sDebugName);
  void Deinitialize();

  ezUInt32 Allocate(ezUInt64 uiUserData, ezUInt32 uiCount);
  ezByteArrayPtr MapForWriting(ezUInt32 uiOffset, ezUInt32& out_uiCount);

  void Resize(ezUInt32 uiNewSize);

  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_Data;
  ezUInt32 m_uiNextOffset = 0;

  struct Allocation
  {
    ezUInt64 m_uiUserData = 0;
    ezUInt32 m_uiCount = 0;
  };

  ezMap<ezUInt32, Allocation> m_Allocations;

  ezDynamicArray<ezGAL::ModifiedRange> m_FreeRanges;
  ezGAL::ModifiedRange m_DirtyRange;

  ezGALBufferCreationDescription m_Desc;

  ezGALBufferHandle m_hBufferForUpload;
  ezGALBufferHandle m_hBufferForRendering;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezString m_sDebugName;
#endif

  static void DeviceEventHandler(const ezGALDeviceEvent& e);
  static ezIdTable<ezGALDynamicBufferHandle::IdType, ezGALDynamicBuffer*> s_Buffers;
};
