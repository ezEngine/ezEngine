
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
ezDelegate<Function> ezMakeDelegate(Function* function)
{
  return ezDelegate<Function>(function);
}

template <typename Method, typename Class>
typename ezMakeDelegateHelper<Method>::DelegateType ezMakeDelegate(Method method, Class* pClass)
{
  return typename ezMakeDelegateHelper<Method>::DelegateType(method, pClass);
}

