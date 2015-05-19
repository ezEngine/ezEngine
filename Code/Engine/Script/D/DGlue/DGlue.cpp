#include <Foundation/Basics.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Delegate.h>

template <typename T>
struct Delegate1 {};

template <typename R1>
struct Delegate1 < R1() > {};

template <typename T1, typename T2>
struct Delegate2 {};

template < typename R1, typename T1, typename T2, typename R2, typename T3, typename T4 >
struct Delegate2<R1(T1, T2), R2(T3, T4)> {};

void DelegateTest1(Delegate1<void()> func)
{

}

void DelegateTest2(Delegate2<int(float, double), int(float, double)> func)
{

}

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
  g_pDefaultScriptReflectionAllocator = EZ_DEFAULT_NEW(ezScriptReflectionAllocator, "DefaultScriptReflectionAllocator", ezFoundation::GetDefaultAllocator());
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