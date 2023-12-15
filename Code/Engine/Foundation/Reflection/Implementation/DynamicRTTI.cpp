#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

bool ezReflectedClass::IsInstanceOf(const ezRTTI* pType) const
{
  return GetDynamicRTTI()->IsDerivedFrom(pType);
}


