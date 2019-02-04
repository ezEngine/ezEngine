#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/VariantAdapter.h>


template <class R, class... Args>
class ezTypedFunctionProperty : public ezAbstractFunctionProperty
{
public:
  ezTypedFunctionProperty(const char* szPropertyName)
      : ezAbstractFunctionProperty(szPropertyName)
  {
  }

  virtual const ezRTTI* GetReturnType() const override { return ezGetStaticRTTI<typename ezCleanType<R>::RttiType>(); }
  virtual ezBitflags<ezPropertyFlags> GetReturnFlags() const override { return ezPropertyFlags::GetParameterFlags<R>(); }

  virtual ezUInt32 GetArgumentCount() const override { return sizeof...(Args); }

  template <std::size_t... I>
  const ezRTTI* GetParameterTypeImpl(ezUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static const ezRTTI* params[] = {ezGetStaticRTTI<typename ezCleanType<typename getArgument<I, Args...>::Type>::RttiType>()..., nullptr};
    return params[uiParamIndex];
  }

  virtual const ezRTTI* GetArgumentType(ezUInt32 uiParamIndex) const override
  {
    return GetParameterTypeImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }

  template <std::size_t... I>
  ezBitflags<ezPropertyFlags> GetParameterFlagsImpl(ezUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static ezBitflags<ezPropertyFlags> params[] = {ezPropertyFlags::GetParameterFlags<typename getArgument<I, Args...>::Type>()...,
                                                   ezPropertyFlags::Void};
    return params[uiParamIndex];
  }

  virtual ezBitflags<ezPropertyFlags> GetArgumentFlags(ezUInt32 uiParamIndex) const override
  {
    return GetParameterFlagsImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <typename FUNC>
class ezFunctionProperty
{
};

template <class CLASS, class R, class... Args>
class ezFunctionProperty<R (CLASS::*)(Args...)> : public ezTypedFunctionProperty<R, Args...>
{
public:
  typedef R (CLASS::*TargetFunction)(Args...);

  ezFunctionProperty(const char* szPropertyName, TargetFunction func)
      : ezTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Member; }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<1>, void* pInstance, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = (CLASS*)pInstance;
    (pTargetInstance->*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    returnValue = ezVariant();
  }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<0>, void* pInstance, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = (CLASS*)pInstance;
    ezVariantAssignmentAdapter<R> returnWrapper(returnValue);
    returnWrapper = (pTargetInstance->*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue) const override
  {
    ExecuteImpl(ezTraitInt<std::is_same<R, void>::value>(), pInstance, returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};


template <class R, class... Args>
class ezFunctionProperty<R (*)(Args...)> : public ezTypedFunctionProperty<R, Args...>
{
public:
  typedef R (*TargetFunction)(Args...);

  ezFunctionProperty(const char* szPropertyName, TargetFunction func)
      : ezTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::StaticMember; }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<1>, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    (*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    returnValue = ezVariant();
  }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<0>, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    ezVariantAssignmentAdapter<R> returnWrapper(returnValue);
    returnWrapper = (*m_Function)(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue) const override
  {
    ExecuteImpl(ezTraitInt<std::is_same<R, void>::value>(), returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};


template <class CLASS, class... Args>
class ezConstructorFunctionProperty : public ezTypedFunctionProperty<CLASS*, Args...>
{
public:
  ezConstructorFunctionProperty()
      : ezTypedFunctionProperty<CLASS*, Args...>("Constructor")
  {
  }

  virtual ezFunctionType::Enum GetFunctionType() const override { return ezFunctionType::Constructor; }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<1>, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    returnValue = CLASS(ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // returnValue = CLASS(static_cast<typename getArgument<I, Args...>::Type>(ezVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
  }

  template <std::size_t... I>
  void ExecuteImpl(ezTraitInt<0>, ezVariant& returnValue, ezArrayPtr<ezVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pInstance = EZ_DEFAULT_NEW(CLASS, ezVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // CLASS* pInstance = EZ_DEFAULT_NEW(CLASS, static_cast<typename getArgument<I, Args...>::Type>(ezVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
    returnValue = pInstance;
  }

  virtual void Execute(void* pInstance, ezArrayPtr<ezVariant> arguments, ezVariant& returnValue) const override
  {
    ExecuteImpl(ezTraitInt<ezIsStandardType<CLASS>::value>(), returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};

