#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

/// \brief [internal] Implements the functionality that can be accessed through ezAbstractFunctionProperty.
template<typename CLASS>
class ezFunctionProperty : public ezAbstractFunctionProperty
{
public:
  typedef void (CLASS::*TargetFunction)();

  /// \brief Constructor
  ezFunctionProperty(const char* szPropertyName, TargetFunction func) : ezAbstractFunctionProperty(szPropertyName)
  {
    m_Function = func;
  }

  /// \brief Calls the proper function on the provided instance.
  virtual void Execute(void* pInstance) const override
  {
    CLASS* pTargetInstance = (CLASS*) pInstance;
    (pTargetInstance->*m_Function)();
  }

private:
  TargetFunction m_Function;
};

