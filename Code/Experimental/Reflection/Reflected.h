#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Basics/Types/Variant.h>
#include <Foundation/Communication/Message.h>
#include "Type.h"

// The 'type graph' can be built 'by hand', but it is easier to use macros

class ezTypeRTTI;

template<class T>
struct ezGetStaticRTTI
{
  static ezTypeRTTI* Type() { return NULL; }
};

template<class T>
ezTypeRTTI* GetStaticRTTI()
{
  return ezGetStaticRTTI<T>::Type();
}

#define EZ_DECLARE_STATIC_REFLECTION(CLASS) \
  template<>\
  struct ezGetStaticRTTI<CLASS> { static ezTypeRTTI* Type() { static ezTypeRTTI s_Type(#CLASS, NULL); return &s_Type; } };

#define EZ_DECLARE_STATIC_REFLECTION_WITH_BASE(CLASS, BASE) \
  template<>\
  struct ezGetStaticRTTI<CLASS> { static ezTypeRTTI* Type() { static ezTypeRTTI s_Type(#CLASS, ezGetStaticRTTI<BASE>::Type()); return &s_Type; } };

#define EZ_ADD_DYNAMIC_REFLECTION(CLASS)                                                        \
  template<class T>                                                                             \
  friend struct Blaaaa;                                                                         \
  virtual const ezTypeRTTI* GetDynamicRTTI() EZ_OVERRIDE   { return GetStaticRTTI<CLASS>(); }   \


class ezReflectedBase;
EZ_DECLARE_STATIC_REFLECTION(ezReflectedBase);

class ezReflectedBase
{
public:
  virtual const ezTypeRTTI* GetDynamicRTTI() { return NULL; }
};

class ezReflectedFunction
{
public:
  virtual ezUInt32 GetParameterCount() const = 0;

  virtual ezVariant::Type::Enum GetParameterType(ezUInt8 uiParam) const = 0;

  virtual void Execute(void* pClass, ezArrayPtr<ezVariant> Params) = 0;

};

template<typename CLASS, typename P0, typename P1>
class ezReflectedFunctionImpl : public ezReflectedFunction
{
public:
  typedef void (CLASS::*WrappedFunc)(P0, P1);

  ezReflectedFunctionImpl(WrappedFunc f) : m_Function(f) 
  {
  }

  virtual ezUInt32 GetParameterCount() const  EZ_OVERRIDE { return 2; }

  ezVariant::Type::Enum GetParameterType(ezUInt8 uiParam) const EZ_OVERRIDE
  {
    switch (uiParam)
    {
    case 0: return (ezVariant::Type::Enum) ezVariant::TypeDeduction<P0>::value;
    case 1: return (ezVariant::Type::Enum) ezVariant::TypeDeduction<P1>::value;
    }

    EZ_REPORT_FAILURE("Cannot get type for parameter %i on a function that only has %i parameters!", uiParam, GetParameterCount());
    return ezVariant::Type::Invalid;
  }

  void Execute(void* pClass, ezArrayPtr<ezVariant> Params) EZ_OVERRIDE
  {
    (((CLASS*) pClass)->*m_Function)(Params[0].ConvertTo<P0>(), Params[1].ConvertTo<P1>());
  }

private:
  WrappedFunc m_Function;
};
