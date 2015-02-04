#include <Foundation/Basics.h>
#include <Foundation/Memory/StackAllocator.h>

class ezScriptReflectionAllocator : public ezStackAllocator < >
{
public:
  ezScriptReflectionAllocator(const char* szName, ezAllocatorBase* pParent)
    : ezStackAllocator(szName, pParent)
  {

  }

  void Reset()
  {
    ezStackAllocator<>::Reset();
  }
};