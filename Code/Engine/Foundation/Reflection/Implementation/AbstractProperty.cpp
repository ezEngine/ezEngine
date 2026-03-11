#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>

ezAbstractProperty ::~ezAbstractProperty()
{
  // To ensure unloading plugins does not leak any heap allocated attributes etc, we need to properly clean up attributes. The assumption is that anything that is deleted here was created using global 'new' when declaring the reflection information inside EZ_BEGIN_PROPERTIES. Thus, any phantom property must ensure this array is cleared before this destructor is called.
  for (auto pAttrib : m_Attributes)
  {
    delete pAttrib;
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_AbstractProperty);
