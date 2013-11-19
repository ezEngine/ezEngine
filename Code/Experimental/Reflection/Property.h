#pragma once

#include <Foundation/Basics.h>

enum ezPropertyType
{
  Invalid,
  Bool,
  Int32,
  Float,
};

template<class T>
class ezPropertyTypeDeduction
{
public:
  static ezPropertyType s_TypeID;
};

template<class T>
ezPropertyType ezPropertyTypeDeduction<T>::s_TypeID = ezPropertyType::Invalid;

template<> ezPropertyType ezPropertyTypeDeduction<bool>::s_TypeID = ezPropertyType::Bool;
template<> ezPropertyType ezPropertyTypeDeduction<int>::s_TypeID = ezPropertyType::Int32;
template<> ezPropertyType ezPropertyTypeDeduction<float>::s_TypeID = ezPropertyType::Float;

class ezAbstractProperty
{
public:
  ezAbstractProperty(const char* szName);

  virtual ezPropertyType GetPropertyType() const = 0;

  const char* GetPropertyName() const { return m_szPropertyName; }

private:
  const char* m_szPropertyName;
};

