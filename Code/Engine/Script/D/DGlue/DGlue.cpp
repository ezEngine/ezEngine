#include <Foundation/Basics.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/StringBuilder.h>

class ezScriptReflectionAllocator : public ezStackAllocator < >
{
public:
  ezScriptReflectionAllocator(const char* szName, ezAllocatorBase* pParent)
    : ezStackAllocator<>(szName, pParent)
  {

  }

  void Reset();
};

void ezScriptReflectionAllocator::Reset()
{
  ezStackAllocator<>::Reset();
}

ezScriptReflectionAllocator* g_pDefaultScriptReflectionAllocator;

ezScriptReflectionAllocator* ezGetDefaultScriptReflectionAllocator()
{
  return g_pDefaultScriptReflectionAllocator;
}

void ezInitDefaultScriptReflectionAllocator()
{
  auto size = sizeof(ezStringBuilder);
  g_pDefaultScriptReflectionAllocator = EZ_DEFAULT_NEW(ezScriptReflectionAllocator)("DefaultScriptReflectionAllocator", ezFoundation::GetDefaultAllocator());
}

void ezDeinitDefaultScriptReflectionAllocator()
{
  EZ_DEFAULT_DELETE(g_pDefaultScriptReflectionAllocator);
}

void ezConstructStringBuilder(ezStringBuilder& builder)
{
  new (&builder) ezStringBuilder();
}

void ezDestroyStringBuilder(ezStringBuilder& builder)
{
  builder.~ezStringBuilder();
}