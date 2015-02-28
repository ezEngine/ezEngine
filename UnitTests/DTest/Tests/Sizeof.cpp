#include <DTest/PCH.h>
#include <Foundation/Types/Delegate.h>

size_t SizeOfDelegate()
{
  return sizeof(ezDelegate<void()>);
}