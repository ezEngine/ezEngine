
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
EZ_ALWAYS_INLINE ezDelegate<Function> ezMakeDelegate(Function* function)
{
  return ezDelegate<Function>(function);
}

template <typename Method, typename Class>
EZ_ALWAYS_INLINE typename ezMakeDelegateHelper<Method>::DelegateType ezMakeDelegate(Method method, Class* pClass)
{
  return typename ezMakeDelegateHelper<Method>::DelegateType(method, pClass);
}

