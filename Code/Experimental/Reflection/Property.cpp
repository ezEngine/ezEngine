#include "Property.h"
#include "Type.h"

ezAbstractProperty::ezAbstractProperty(const char* szName)
{
  m_szPropertyName = szName;
}



const ezTypeRTTI* ezArrayProperty::GetPropertyType() const
{
  static ezTypeRTTI t("ezArrayProperty", NULL);

  return &t;
}
