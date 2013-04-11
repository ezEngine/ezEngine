#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/AtomicInteger.h>

namespace ezMemoryPolicies
{
  class EZ_FOUNDATION_DLL ezSimpleTracking
  {
  public:
    void AddAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize)
    {
      m_uiNumAllocations.Increment();
      m_uiAllocationSize.Add(uiAllocatedSize);
      m_uiUsedMemorySize.Add(uiUsedMemorySize);
    }

    void RemoveAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize)
    {
      m_uiNumDeallocations.Increment();
      m_uiAllocationSize.Subtract(uiAllocatedSize);
      m_uiUsedMemorySize.Subtract(uiUsedMemorySize);
    }

    EZ_FORCE_INLINE void DumpMemoryLeaks() const { }

    ezUInt64 GetNumAllocations() const { return m_uiNumAllocations; }
    ezUInt64 GetNumDeallocations() const { return m_uiNumDeallocations; }
    ezUInt64 GetNumLiveAllocations() const { return m_uiNumAllocations - m_uiNumDeallocations; }
    ezUInt64 GetAllocationSize() const { return m_uiAllocationSize; }      
    ezUInt64 GetUsedMemorySize() const { return m_uiUsedMemorySize; }

  private:
    ezAtomicInteger64 m_uiNumAllocations;
    ezAtomicInteger64 m_uiNumDeallocations;
    ezAtomicInteger64 m_uiAllocationSize;
    ezAtomicInteger64 m_uiUsedMemorySize;
  };

  class EZ_FOUNDATION_DLL ezStackTracking : public ezSimpleTracking
  {
  private:
    struct TrackingInfo
    {
      EZ_DECLARE_POD_TYPE();

      size_t uiAllocatedSize;
      void** pTrace;
    };

    ezHashTable<void*, TrackingInfo> m_trackings;

  public:
    ezStackTracking();
    ~ezStackTracking();

    void AddAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize);
    void RemoveAllocation(void* ptr, size_t uiAllocatedSize, size_t uiUsedMemorySize);

    void DumpMemoryLeaks() const;
  };
}
