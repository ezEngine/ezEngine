#include <Foundation/Basics.h>
#include <Foundation/Memory/StackAllocator.h>

class ezScriptReflectionAllocator : public ezStackAllocator < >
{
public:
  ezScriptReflectionAllocator(const char* szName, ezAllocatorBase* pParent)
    : ezStackAllocator<>(szName, pParent)
  {

  }

  void Reset()
  {
    ezStackAllocator<>::Reset();
  }
};

ezScriptReflectionAllocator* g_pDefaultScriptReflectionAllocator;

ezScriptReflectionAllocator* ezGetDefaultScriptReflectionAllocator()
{
  return g_pDefaultScriptReflectionAllocator;
}

void ezInitDefaultScriptReflectionAllocator()
{
  g_pDefaultScriptReflectionAllocator = EZ_DEFAULT_NEW(ezScriptReflectionAllocator)("DefaultScriptReflectionAllocator", ezFoundation::GetDefaultAllocator());
}

void ezDeinitDefaultScriptReflectionAllocator()
{
  EZ_DEFAULT_DELETE(g_pDefaultScriptReflectionAllocator);
}