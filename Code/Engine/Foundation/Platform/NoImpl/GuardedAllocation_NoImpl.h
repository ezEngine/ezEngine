#include <Foundation/Memory/Policies/GuardedAllocation.h>

namespace ezMemoryPolicies
{
  ezAllocPolicyGuarding::ezAllocPolicyGuarding(ezAllocator* pParent)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  void* ezAllocPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void ezAllocPolicyGuarding::Deallocate(void* ptr)
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }
} // namespace ezMemoryPolicies
