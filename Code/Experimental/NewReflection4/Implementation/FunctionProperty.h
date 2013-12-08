#pragma once

/// \file

#include "AbstractProperty.h"

template<typename CLASS>
class ezFunctionProperty : public ezAbstractFunctionProperty
{
public:
  typedef void (CLASS::*TargetFunction)();

  ezFunctionProperty(const char* szPropertyName, TargetFunction func) : ezAbstractFunctionProperty(szPropertyName)
  {
    m_Function = func;
  }

  virtual void Execute(void* pInstance) const EZ_OVERRIDE
  {
    CLASS* pTargetInstance = (CLASS*) pInstance;
    (pTargetInstance->*m_Function)();
  }

private:
  TargetFunction m_Function;
};