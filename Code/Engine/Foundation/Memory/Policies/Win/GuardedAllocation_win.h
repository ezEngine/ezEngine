
namespace ezMemoryPolicies
{
  struct AlloctionMetaData
  {
    AlloctionMetaData()
    {
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_magic); ++i)
      {
        m_magic[i] = 0x12345678;
      }
    }

    ~AlloctionMetaData()
    {
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_magic); ++i)
      {
        EZ_ASSERT_DEV(m_magic[i] == 0x12345678, "Magic value has been overwritten. This might be the result of a buffer underrun!");
      }
    }

    size_t m_uiSize;
    ezUInt32 m_magic[32];
  };

  ezGuardedAllocation::ezGuardedAllocation(ezAllocatorBase* pParent)
  {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_uiPageSize = sysInfo.dwPageSize;
  }


  void* ezGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    EZ_ASSERT_DEV(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");
    uiAlign = ezMath::Max<size_t>(uiAlign, EZ_ALIGNMENT_MINIMUM);

    size_t uiAlignedSize = ezMemoryUtils::AlignSize(uiSize, uiAlign);
    size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);

    // align to full pages and add one page in front and one in back
    size_t uiPageSize = m_uiPageSize;
    size_t uiFullPageSize = ezMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
    void* pMemory = VirtualAlloc(nullptr, uiFullPageSize + 2 * uiPageSize, MEM_RESERVE, PAGE_NOACCESS);
    EZ_ASSERT_DEV(pMemory != nullptr, "Could not reserve memory pages. Error Code '{0}'", ((ezUInt32)::GetLastError()));

    // add one page and commit the payload pages
    pMemory = ezMemoryUtils::AddByteOffset(pMemory, uiPageSize);
    void* ptr = VirtualAlloc(pMemory, uiFullPageSize, MEM_COMMIT, PAGE_READWRITE);
    EZ_ASSERT_DEV(ptr != nullptr, "Could not commit memory pages. Error Code '{0}'", ((ezUInt32)::GetLastError()));

    // store information in meta data
    AlloctionMetaData* metaData = ezMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(ptr), uiFullPageSize - uiTotalSize);
    ezMemoryUtils::Construct(metaData, 1);
    metaData->m_uiSize = uiAlignedSize;

    // finally add offset to the actual payload
    ptr = ezMemoryUtils::AddByteOffset(metaData, sizeof(AlloctionMetaData));
    return ptr;
  }

  void ezGuardedAllocation::Deallocate(void* ptr)
  {
    ezLock<ezMutex> lock(m_mutex);

    if (!m_AllocationsToFreeLater.CanAppend())
    {
      void* pMemory = m_AllocationsToFreeLater.PeekFront();
      EZ_VERIFY(::VirtualFree(pMemory, 0, MEM_RELEASE), "Could not free memory pages. Error Code '%d'", ((ezUInt32)::GetLastError()));

      m_AllocationsToFreeLater.PopFront();
    }

    // Retrieve info from meta data first.
    AlloctionMetaData* metaData = ezMemoryUtils::AddByteOffset(static_cast<AlloctionMetaData*>(ptr), -((ptrdiff_t)sizeof(AlloctionMetaData)));
    size_t uiAlignedSize = metaData->m_uiSize;

    ezMemoryUtils::Destruct(metaData, 1);

    // Decommit the pages but do not release the memory yet so use-after-free can be detected.
    size_t uiPageSize = m_uiPageSize;
    size_t uiTotalSize = uiAlignedSize + sizeof(AlloctionMetaData);
    size_t uiFullPageSize = ezMemoryUtils::AlignSize(uiTotalSize, uiPageSize);
    ptr = ezMemoryUtils::AddByteOffset(ptr, ((ptrdiff_t)uiAlignedSize) - uiFullPageSize);

    EZ_VERIFY(::VirtualFree(ptr, uiFullPageSize, MEM_DECOMMIT), "Could not decommit memory pages. Error Code '%d'", ((ezUInt32)::GetLastError()));

    // Finally store the allocation so we can release it later
    void* pMemory = ezMemoryUtils::AddByteOffset(ptr, -((ptrdiff_t)uiPageSize));
    m_AllocationsToFreeLater.PushBack(pMemory);
  }

}
