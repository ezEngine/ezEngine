
namespace ezMemoryPolicies
{
  ezGuardedAllocation::ezGuardedAllocation(ezAllocatorBase* pParent)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }


  void* ezGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void ezGuardedAllocation::Deallocate(void* ptr)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
}

