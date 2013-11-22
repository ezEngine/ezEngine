#pragma once

#include <Foundation/Basics.h>





class ezTypeRTTI;

class ezAbstractProperty
{
public:
  ezAbstractProperty(const char* szName);

  virtual const ezTypeRTTI* GetPropertyType() const = 0;

  const char* GetPropertyName() const { return m_szPropertyName; }

  virtual void* GetPropertyPointer(const void* pReflected) const = 0;

private:
  const char* m_szPropertyName;
};

