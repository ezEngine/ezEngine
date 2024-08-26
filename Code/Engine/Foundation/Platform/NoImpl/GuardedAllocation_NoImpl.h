#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>

ezAllocPolicyGuarding::ezAllocPolicyGuarding(ezAllocator* pParent)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  EZ_IGNORE_UNUSED(m_uiPageSize);
  EZ_IGNORE_UNUSED(m_Mutex);
  EZ_IGNORE_UNUSED(m_AllocationsToFreeLater);
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
