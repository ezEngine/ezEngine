#pragma once

#include <Foundation/Basics.h>

class ezRTTI;

class ezAbstractProperty
{
public:
  enum PropertyCategory
  {
    Member,
    Function,
    Array
  };

  ezAbstractProperty(const char* szPropertyName)
  {
    m_szPropertyName = szPropertyName;
  }

  const char* GetPropertyName() const { return m_szPropertyName; }

  virtual PropertyCategory GetCategory() const = 0;

private:
  const char* m_szPropertyName;
};

class ezAbstractMemberProperty : public ezAbstractProperty
{
public:
  ezAbstractMemberProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  virtual ezAbstractProperty::PropertyCategory GetCategory() const EZ_OVERRIDE { return ezAbstractProperty::Member; }

  virtual const ezRTTI* GetPropertyType() const = 0;

  virtual void* GetPropertyPointer() const = 0;

  virtual bool IsReadOnly() const = 0;
};

class ezAbstractFunctionProperty : public ezAbstractProperty
{
public:
  ezAbstractFunctionProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  virtual ezAbstractProperty::PropertyCategory GetCategory() const EZ_OVERRIDE { return ezAbstractProperty::Function; }

  virtual void Execute(void* pInstance) const = 0;

};

class ezAbstractArrayProperty : public ezAbstractProperty
{
public:
  ezAbstractArrayProperty(const char* szPropertyName) : ezAbstractProperty(szPropertyName)
  {
  }

  virtual ezAbstractProperty::PropertyCategory GetCategory() const EZ_OVERRIDE { return ezAbstractProperty::Array; }

  virtual const ezRTTI* GetElementType() const = 0;

};