#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/VariantAdapter.h>


template<class R, class...Args>
class ezTypedFunctionProperty : public ezAbstractFunctionProperty
{
public:
  ezTypedFunctionProperty(const char* szPropertyName) : ezAbstractFunctionProperty(szPropertyName)
  {
  }

  virtual const ezRTTI* GetReturnType() const override
  {
    return ezGetStaticRTTI<typename ezCleanType<R>::RttiType>();
  }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override
  {
    return ezPropertyFlags::GetParameterFlags<R>();
  }

  virtual ezUInt32 GetArgumentCount() const override
  {
    return sizeof...(Args);
  }

  template<std::size_t... I>
  const ezRTTI* GetParameterTypeImpl(ezUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    static const ezRTTI* params[] = { ezGetStaticRTTI<typename ezCleanType<typename getArgument<I, Args...>::Type>::RttiType>()... };
    return params[uiParamIndex];
  }

  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override
  {
    return GetParameterTypeImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }

  template<std::size_t... I>
  ezBitflags<ezPropertyFlags> GetParameterFlagsImpl(ezUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    static ezBitflags<ezPropertyFlags> params[] = { ezPropertyFlags::GetParameterFlags<typename getArgument<I, Args...>::Type>()... };
    return params[uiParamIndex];
  }

  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override
  {
    return GetParameterFlagsImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }
};

template<typename FUNC>
class ezFunctionProperty
{
};

template<class CLASS, class R, class...Args>
class ezFunctionProperty<R(CLASS::*)(Args...)> : public ezTypedFunctionProperty<R, Args...>
{
public:
  typedef R(CLASS::*TargetFunction)(Args...);

  ezFunctionProperty(const char* szPropertyName, TargetFunction func)
    : ezTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual ezFunctionPropertyType::Enum GetFunctionType() const override
  {
    return ezFunctionPropertyType::Member;
  }

  template<std::size_t... I>
  void ExecuteImpl(ezTraitInt<1>, void* pInstance, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = (CLASS*)pInstance;
    (pTargetInstance->*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    returnValue = ezVariant();
  }

  template<std::size_t... I>
  void ExecuteImpl(ezTraitInt<0>, void* pInstance, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = (CLASS*)pInstance;
    ezVariantAssignmentAdapter<R> returnWrapper(returnValue);
    returnWrapper = (pTargetInstance->*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue) const override
  {
    CLASS* pTargetInstance = (CLASS*)pInstance;
    ExecuteImpl(ezTraitInt<std::is_same<R, void>::value>(), pInstance, returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};
