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


class ezArrayProperty : public ezAbstractProperty
{
public:

  ezArrayProperty(const char* szName) : ezAbstractProperty(szName)
  {
  }

  virtual const ezTypeRTTI* GetPropertyType() const;

  virtual const ezTypeRTTI* GetElementType() const = 0;

  virtual ezUInt32 GetCount(const void* pProperty) const = 0;

  virtual void* GetElement(const void* pProperty, ezUInt32 at) = 0;

};



